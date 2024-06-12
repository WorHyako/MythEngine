#include <RHI/OpenGL/OpenGLStudyRender.hpp>

#include <glad/gl.h>
#include "GLFW/glfw3.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Core/GL.h"

#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include "UserInput/GLFW/GLFWUserInput.hpp"
#include "OpenGLBaseShader.hpp"
#include <Scene/VtxData.hpp>
#include <Camera/TestCamera.hpp>

#include <RHI/OpenGL/Framework/UtilsGLImGui.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <Bitmap.hpp>
#include <UtilsCubemap.hpp>
#include <Utils/UtilsFPS.hpp>
#include <Filesystem/FilesystemUtilities.hpp>

#include "stb_image.h"
#include "stb_image_write.h"

static const char* shaderCodeVertex = R"(
#version 460 core
layout (std140, binding = 0) uniform PerFrameData{
	uniform mat4 MVP;
	uniform int isWireframe;
};
layout (location=0) out vec2 uv;
layout (location=1) out int Wireframe;
const vec3 pos[8] = vec3[8](
	vec3(-1.0,-1.0, 1.0),
	vec3( 1.0,-1.0, 1.0),
	vec3( 1.0, 1.0, 1.0),
	vec3(-1.0, 1.0, 1.0),

	vec3(-1.0,-1.0,-1.0),
	vec3( 1.0,-1.0,-1.0),
	vec3( 1.0, 1.0,-1.0),
	vec3(-1.0, 1.0,-1.0)
);
const vec2 tc[8] = vec2[8](
	vec2(0.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0),

	vec2(1.0, 1.0),
	vec2(1.0, 0.0),
	vec2(0.0, 0.0),
	vec2(0.0, 1.0)
);
const vec3 col[8] = vec3[8](
	vec3( 1.0, 0.0, 0.0),
	vec3( 0.0, 1.0, 0.0),
	vec3( 0.0, 0.0, 1.0),
	vec3( 1.0, 1.0, 0.0),

	vec3( 1.0, 1.0, 0.0),
	vec3( 0.0, 0.0, 1.0),
	vec3( 0.0, 1.0, 0.0),
	vec3( 1.0, 0.0, 0.0)
);
const int indices[36] = int[36](
	// front
	0, 1, 2, 2, 3, 0,
	// right
	1, 5, 6, 6, 2, 1,
	// back
	7, 6, 5, 5, 4, 7,
	// left
	4, 0, 3, 3, 7, 4,
	// bottom
	4, 5, 1, 1, 0, 4,
	// top
	3, 2, 6, 6, 7, 3
);
void main()
{
	int idx = indices[gl_VertexID];
	gl_Position = MVP * vec4(pos[idx], 1.0);
	uv = tc[idx];
	Wireframe = isWireframe;
}
)";

static const char* shaderCodeFragment = R"(
#version 460 core
layout (location=0) in vec2 uv;
layout (location=0) out vec4 out_FragColor;
uniform sampler2D texture0;
void main()
{
	out_FragColor = texture(texture0, uv);
};
)";

struct PerFrameData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 cameraPos;
    float tesselationScale;
    //int isWireframe;
};

float tessellationScale = 1.0f;

CameraPositioner_FirstPerson positioner(vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
TestCamera testCamera = TestCamera(positioner);

struct DrawElementsIndirectCommand
{
    GLuint count_;
    GLuint instanceCount_;
    GLuint firstIndex_;
    GLuint baseVertex_;
    GLuint baseInstance_;
};

class GLMeshStudy final
{
public:
    GLMeshStudy(const MeshFileHeader& header, const Mesh* meshes, const uint32_t* indices, const float* vertexData)
        : numIndices(header.indexDataSize / sizeof(uint32_t))
        , bufferIndices_(header.indexDataSize, indices, 0)
        , bufferVertices_(header.vertexDataSize, vertexData, 0)
        , bufferIndirect_(sizeof(DrawElementsIndirectCommand)* header.meshCount + sizeof(GLsizei), nullptr, GL_DYNAMIC_STORAGE_BIT)
    {
        glCreateVertexArrays(1, &vao_);
        glVertexArrayElementBuffer(vao_, bufferIndices_.getHandle());
        glVertexArrayVertexBuffer(vao_, 0, bufferVertices_.getHandle(), 0, sizeof(vec3) + sizeof(vec3) + sizeof(vec2));
        // position
        glEnableVertexArrayAttrib(vao_, 0);
        glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(vao_, 0, 0);
        // uv
        glEnableVertexArrayAttrib(vao_, 1);
        glVertexArrayAttribFormat(vao_, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vec3));
        glVertexArrayAttribBinding(vao_, 1, 0);
        // normal
        glEnableVertexArrayAttrib(vao_, 2);
        glVertexArrayAttribFormat(vao_, 2, 3, GL_FLOAT, GL_TRUE, sizeof(vec3) + sizeof(vec2));
        glVertexArrayAttribBinding(vao_, 2, 0);

        std::vector<uint8_t> drawCommands;

        const GLsizei numCommands = (GLsizei)header.meshCount;

        drawCommands.resize(sizeof(DrawElementsIndirectCommand) * numCommands + sizeof(GLsizei));

        // store the number of draw commands in the very beginning of the buffer
        memcpy(drawCommands.data(), &numCommands, sizeof(numCommands));

        DrawElementsIndirectCommand* cmd = std::launder(
            reinterpret_cast<DrawElementsIndirectCommand*>(drawCommands.data() + sizeof(GLsizei))
        );

        // prepare indirect commands buffer
        for (uint32_t i = 0; i != numCommands; i++)
        {
            DrawElementsIndirectCommand drawCmd{};
            drawCmd.count_ = meshes[i].getLODIndicesCount(0);
            drawCmd.instanceCount_ = 1,
                drawCmd.firstIndex_ = meshes[i].indexOffset;
            drawCmd.baseVertex_ = meshes[i].vertexOffset;
            drawCmd.baseInstance_ = 0;
            *cmd++ = drawCmd;
        }

        glNamedBufferSubData(bufferIndirect_.getHandle(), 0, drawCommands.size(), drawCommands.data());
        glBindVertexArray(vao_);
    }

    void draw(const MeshFileHeader& header) const
    {
        glBindVertexArray(vao_);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, bufferIndirect_.getHandle());
        glBindBuffer(GL_PARAMETER_BUFFER, bufferIndirect_.getHandle());
        glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)sizeof(GLsizei), 0, (GLsizei)header.meshCount, 0);
    }

    ~GLMeshStudy()
    {
        glDeleteVertexArrays(1, &vao_);
    }

    GLMeshStudy(const GLMeshStudy&) = delete;
    GLMeshStudy(GLMeshStudy&&) = default;

private:
    GLuint vao_;
    uint32_t numIndices;

    GLBuffer bufferIndices_;
    GLBuffer bufferVertices_;
    GLBuffer bufferIndirect_;
};

OpenGLStudyRender::OpenGLStudyRender(GLApp* app)
	: OpenGLBaseRender(app)
{}

int OpenGLStudyRender::draw()
{
    /*3D Rendering Cookbook*/
    GLFWUserInput& input = GLFWUserInput::GetInput();
    input.positioner = &positioner;

    GL4API api;

    GetAPI4(&api, [](const char* func)->void* {return (void*)glfwGetProcAddress(func); });
    InjectAPITracer4(&api);

    struct VertexData
    {
        glm::vec3 pos;
        glm::vec3 n;
        glm::vec2 tc;
    };

    //glEnable(GL_POLYGON_OFFSET_LINE);
    //glPolygonOffset(-1.0f, -1.0f);

    GLShader vertexPullingVertexShader((FilesystemUtilities::GetShadersDir() + "OpenGL/VertexPulling/GL02.vert").c_str());
    GLShader vertexPullingGeometryShader((FilesystemUtilities::GetShadersDir() + "OpenGL/VertexPulling/GL02.geom").c_str());
    GLShader vertexPullingFragmentShader((FilesystemUtilities::GetShadersDir() + "OpenGL/VertexPulling/GL02.frag").c_str());
    GLProgram vertexPullingShaderProgram(vertexPullingVertexShader, vertexPullingGeometryShader, vertexPullingFragmentShader);

    glDeleteShader(vertexPullingVertexShader.getHandle());
    glDeleteShader(vertexPullingGeometryShader.getHandle());
    glDeleteShader(vertexPullingFragmentShader.getHandle());

    GLShader shdModelVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/Cubemap/CubemapModel.vert").c_str());
    GLShader shdModelFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/Cubemap/CubemapModel.frag").c_str());
    GLProgram progModel(shdModelVertex, shdModelFragment);

    GLShader shdCubeVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/Cubemap/CubemapCube.vert").c_str());
    GLShader shdCubeFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/Cubemap/CubemapCube.frag").c_str());
    GLProgram progCube(shdCubeVertex, shdCubeFragment);

    GLShader shdGridVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.vert").c_str());
    GLShader shdGridFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.frag").c_str());
    GLProgram progGrid(shdGridVertex, shdGridFragment);

    GLShader shdIndirectVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/MultiDrawIndirect/MultiDrawIndirect.vert").c_str());
    GLShader shdIndirectGeometry((FilesystemUtilities::GetShadersDir() + "OpenGL/MultiDrawIndirect/MultiDrawIndirect.geom").c_str());
    GLShader shdIndirectFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/MultiDrawIndirect/MultiDrawIndirect.frag").c_str());
    GLProgram programIndirect(shdIndirectVertex, shdIndirectGeometry, shdIndirectFragment);

    GLShader shdTessellationVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/Tessellation/Tessellation.vert").c_str());
    GLShader shdTessellationTessControl((FilesystemUtilities::GetShadersDir() + "OpenGL/Tessellation/Tessellation.tesc").c_str());
    GLShader shdTessellationTessEvaluation((FilesystemUtilities::GetShadersDir() + "OpenGL/Tessellation/Tessellation.tese").c_str());
    GLShader shdTessellationGeometry((FilesystemUtilities::GetShadersDir() + "OpenGL/Tessellation/Tessellation.geom").c_str());
    GLShader shdTessellationFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/Tessellation/Tessellation.frag").c_str());
    GLProgram programTessellation(shdTessellationVertex, shdTessellationTessControl, shdTessellationTessEvaluation, shdTessellationGeometry, shdTessellationFragment);

    /*const GLuint shaderFragment = glCreateShader(GL_FRAGMENT_SHADER);
    api.glShaderSource(shaderFragment, 1, &shaderCodeFragment, nullptr);
    api.glCompileShader(shaderFragment);

    const GLuint vs = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &shaderCodeVertex);
    const GLuint fs = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &shaderCodeFragment);
    GLuint pipeline;
    glCreateProgramPipelines(1, &pipeline);
    glUseProgramStages(pipeline, GL_VERTEX_SHADER, vs);
    glUseProgramStages(pipeline, GL_FRAGMENT_SHADER, fs);
    glBindProgramPipeline(pipeline);

    GLuint triangleVAO;
    glCreateVertexArrays(1, &triangleVAO);
    glBindVertexArray(triangleVAO);*/

    const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

    GLuint perFrameDataBuf;
    glCreateBuffers(1, &perFrameDataBuf);
    glNamedBufferStorage(perFrameDataBuf, kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuf, 0, kUniformBufferSize);

    const aiScene* duckModelScene = aiImportFile((FilesystemUtilities::GetResourcesDir() + "objects/rubber_duck/scene.gltf").c_str(), aiProcess_Triangulate);
    if (!duckModelScene || !duckModelScene->HasMeshes())
    {
        printf("Unable to load data/rubber_duck/scene.gltf\n");
        exit(255);
    }

    const aiMesh* duckMesh = duckModelScene->mMeshes[0];
    std::vector<VertexData> duckVertices;
    for (unsigned int i = 0; i != duckMesh->mNumVertices; i++)
    {
        const aiVector3D v = duckMesh->mVertices[i];
        const aiVector3D n = duckMesh->mNormals[i];
        const aiVector3D t = duckMesh->mTextureCoords[0][i];
        duckVertices.push_back({ glm::vec3(v.x, v.z, v.y), glm::vec3(n.x, n.y, n.z), glm::vec2(t.x, t.y) });
    }

    std::vector<unsigned int> duckIndices;
    for (unsigned i = 0; i != duckMesh->mNumFaces; i++)
    {
        for (unsigned j = 0; j != 3; j++)
            duckIndices.push_back(duckMesh->mFaces[i].mIndices[j]);
    }
    aiReleaseImport(duckModelScene);

    const size_t kSizeIndices = sizeof(unsigned int) * duckIndices.size();
    const size_t kSizeVertices = sizeof(VertexData) * duckVertices.size();

    // indices
    GLuint dataIndices;
    glCreateBuffers(1, &dataIndices);
    glNamedBufferStorage(dataIndices, kSizeIndices, duckIndices.data(), 0);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, dataIndices);
    GLuint duckVAO;
    glCreateVertexArrays(1, &duckVAO);
    glBindVertexArray(duckVAO);
    glVertexArrayElementBuffer(duckVAO, dataIndices);

    // vertices
    GLuint dataVertices;
    glCreateBuffers(1, &dataVertices);
    glNamedBufferStorage(dataVertices, kSizeVertices, duckVertices.data(), 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dataVertices);

    // model matrices
    GLuint testModelMatrices;
    glCreateBuffers(1, &testModelMatrices);
    glNamedBufferStorage(testModelMatrices, sizeof(mat4) * 2, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, testModelMatrices);

    // grid vao
    GLuint gridVAO;
    glCreateVertexArrays(1, &gridVAO);
    glBindVertexArray(gridVAO);

    MeshData meshData;
    const MeshFileHeader header = loadMeshData((FilesystemUtilities::GetResourcesDir() + "Meshes/test.meshes").c_str(), meshData);

    GLMeshStudy mesh(header, meshData.meshes_.data(), meshData.indexData_.data(), meshData.vertexData_.data());

    GLuint texture;
    {
        stbi_set_flip_vertically_on_load(false);
        int w, h, comp;
        const uint8_t* img = stbi_load((FilesystemUtilities::GetResourcesDir() + "objects/rubber_duck/textures/Duck_baseColor.png").c_str(), &w, &h, &comp, 3);

        glCreateTextures(GL_TEXTURE_2D, 1, &texture);
        glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureStorage2D(texture, 1, GL_RGB8, w, h);
        glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);
        glBindTextures(0, 1, &texture);

        stbi_image_free((void*)img);
    }

    GLuint cubemapTex;
    {
        int w, h, comp;
        const float* img = stbi_loadf((FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k.hdr").c_str(), &w, &h, &comp, 3);
        Bitmap in(w, h, comp, eBitmapFormat_Float, img);
        Bitmap out = convertEquirectangularMapToVerticalCross(in);
        stbi_image_free((void*)img);

        stbi_write_hdr("screenshot.hdr", out.w_, out.h_, out.comp_, reinterpret_cast<const float*>(out.data_.data()));

        Bitmap cubemap = convertVerticalCrossToCubeMapFaces(out);

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemapTex);
        glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(cubemapTex, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(cubemapTex, GL_TEXTURE_MAX_LEVEL, 0);
        glTextureParameteri(cubemapTex, GL_TEXTURE_MAX_LEVEL, 0);
        glTextureParameteri(cubemapTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(cubemapTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(cubemapTex, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
        glTextureStorage2D(cubemapTex, 1, GL_RGB32F, cubemap.w_, cubemap.h_);
        const uint8_t* data = cubemap.data_.data();

        for (unsigned i = 0; i != 6; ++i)
        {
            glTextureSubImage3D(cubemapTex, 0, 0, 0, i, cubemap.w_, cubemap.h_, 1, GL_RGB, GL_FLOAT, data);
            data += cubemap.w_ * cubemap.h_ * cubemap.comp_ * Bitmap::getBytesPerComponent(cubemap.fmt_);
        }
        glBindTextures(1, 1, &cubemapTex);
    }

    double timeStamp = glfwGetTime();
    float deltaSeconds = 0.0f;

    FramesPerSecondCounter fpsCounter(0.5f);

    ImGuiGLRenderer rendererUI;

    while (!glfwWindowShouldClose(window_))
    {
        ImGuiIO& io = ImGui::GetIO();

        fpsCounter.tick(deltaSeconds);

        positioner.update(deltaSeconds, input.mouseState->pos, input.mouseState->pressedLeft && !io.WantCaptureMouse);

        const double newTimeStamp = glfwGetTime();
        deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
        timeStamp = newTimeStamp;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3D Rendering Cookbook
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        const float ratio = width / (float)height;

        glViewport(0, 0, width, height);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        const glm::mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
        const glm::mat4 view = testCamera.getViewMatrix();

        glBindVertexArray(duckVAO);
        {
            const PerFrameData perFrameData = { view , p, glm::vec4(testCamera.getPosition(), 1.0f), tessellationScale };
            glNamedBufferSubData(perFrameDataBuf, 0, kUniformBufferSize, &perFrameData);

            const mat4 Matrices[2]
            {
                glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, -0.5f, -1.5f)), (float)glfwGetTime() * 0.1f, vec3(0.0f, 1.0f, 0.0f)),
                glm::scale(mat4(1.0f), vec3(10.0f))
            };

            glNamedBufferSubData(testModelMatrices, 0, sizeof(mat4) * 2, &Matrices);

            /*const mat4 s = glm::scale(mat4(1.0f), vec3(10.0f));
            const mat4 m = s * glm::rotate(glm::translate(mat4(1.0f), vec3(0.0f, -0.5f, -1.5f)), (float) glfwGetTime() * 0.1f, vec3(0.0f, 1.0f, 0.0f));
            glNamedBufferSubData(testModelMatrices, 0, sizeof(mat4), value_ptr(m));*/

            programTessellation.useProgram();
            glBindTextures(0, 1, &texture);
            glDrawElements(GL_PATCHES, static_cast<unsigned>(duckIndices.size()), GL_UNSIGNED_INT, nullptr);
            //glDrawElementsInstancedBaseVertexBaseInstance(GL_PATCHES, static_cast<unsigned>(duckIndices.size()), GL_UNSIGNED_INT, nullptr, 1, 0, 0);
            //progModel.useProgram();
            //glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, static_cast<unsigned>(duckIndices.size()), GL_UNSIGNED_INT, nullptr, 1, 0, 0);
            //glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(duckIndices.size()), GL_UNSIGNED_INT, nullptr);
            //glDrawArrays(GL_TRIANGLES, 0, duckIndices.size());
        }

        //progCube.useProgram();
        //glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 36, 1, 1);
        ////glDrawArrays(GL_TRIANGLES, 0, 36);

        //glEnable(GL_DEPTH_TEST);
        //glDisable(GL_BLEND);
        //programIndirect.useProgram();
        //mesh.draw(header);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(gridVAO);

        progGrid.useProgram();
        glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);

        io.DisplaySize = ImVec2((float)width, (float)height);
        ImGui::NewFrame();
        ImGui::SliderFloat("Tessellation scale", &tessellationScale, 1.0f, 2.0f, "%.1f");
        ImGui::Render();
        rendererUI.render(width, height, ImGui::GetDrawData());

        //vertexPullingShaderProgram.useProgram();
        //glBindTextures(0, 1, &texture);

        //PerFrameData perFrameData;
        //perFrameData.mvp = p * m;
        ////perFrameData.isWireframe = false;
        //glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuf, 0, kBufferSize);
        //glNamedBufferSubData(perFrameDataBuf, 0, kBufferSize, &perFrameData);
        ////glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        //glBindVertexArray(duckVAO);
        //glVertexArrayElementBuffer(duckVAO, dataIndices);
        //glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(duckIndices.size()), GL_UNSIGNED_INT, nullptr);

        /*perFrameData.isWireframe = true;
        glNamedBufferSubData(perFrameDataBuf, 0, kBufferSize, &perFrameData);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLES, 0, 36);*/

        if(app_) app_->swapBuffers();
    }

    glDeleteBuffers(1, &dataIndices);
    glDeleteBuffers(1, &dataVertices);
    glDeleteBuffers(1, &perFrameDataBuf);
    glDeleteVertexArrays(1, &duckVAO);
    glDeleteVertexArrays(1, &gridVAO);
}
