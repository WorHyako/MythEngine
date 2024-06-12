#include "OpenGL_GLTF_PBRRender.hpp"

#include <glad/gl.h>
#include "GLFW/glfw3.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Core/GL.h"

#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include <UserInput/GLFW/GLFWUserInput.hpp>
#include <RHI/OpenGL/Framework/GLShader.hpp>
#include <RHI/OpenGL/Framework/GLTexture.hpp>
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


struct PerFrameData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 cameraPos;
};

class GLMeshPVPPBR final
{
public:
    GLMeshPVPPBR(const uint32_t* indices, uint32_t indicesSize, const float* vertexData, uint32_t verticesSize)
        : numIndices(indicesSize / sizeof(uint32_t))
        , bufferIndices_(indicesSize, indices, 0)
        , bufferVertices_(verticesSize, vertexData, 0)
    {
        glCreateVertexArrays(1, &vao_);
        glVertexArrayElementBuffer(vao_, bufferIndices_.getHandle());
    }

    void draw() const
    {
        glBindVertexArray(vao_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bufferVertices_.getHandle());
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(numIndices), GL_UNSIGNED_INT, nullptr);
    }

    ~GLMeshPVPPBR()
    {
        glDeleteVertexArrays(1, &vao_);
    }

private:
    GLuint vao_;
    uint32_t numIndices;

    GLBuffer bufferIndices_;
    GLBuffer bufferVertices_;
};

OpenGL_GLTF_PBRRender::OpenGL_GLTF_PBRRender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(0.0f, 6.0f, 11.0f), vec3(0.0f, 4.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{}

int OpenGL_GLTF_PBRRender::draw()
{
    /*3D Rendering Cookbook*/

    GLShader shdGridVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.vert").c_str());
    GLShader shdGridFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.frag").c_str());
    GLProgram progGrid(shdGridVertex, shdGridFragment);

    const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

    GLBuffer perFrameDataBuffer(kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle(), 0, kUniformBufferSize);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    GLShader shaderVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/GLTF_PBRShader/GLTF_PBRShader.vert").c_str());
    GLShader shaderFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/GLTF_PBRShader/GLTF_PBRShader.frag").c_str());
    GLProgram program(shaderVertex, shaderFragment);

    GLFWUserInput& input = GLFWUserInput::GetInput();
    input.positioner = positioner_;

    const aiScene* scene = aiImportFile((FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/DamagedHelmet.gltf").c_str(), aiProcess_Triangulate);

    if (!scene || !scene->HasMeshes())
    {
        printf("Unable to load objects/DamagedHelmet/glTF/DamagedHelmet.gltf\n");
        exit(255);
    }

    struct VertexData
    {
        glm::vec3 pos;
        glm::vec3 n;
        glm::vec2 tc;
    };

    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
    {
        const aiMesh* mesh = scene->mMeshes[0];
        for (unsigned int i = 0; i != mesh->mNumVertices; i++)
        {
            const aiVector3D v = mesh->mVertices[i];
            const aiVector3D n = mesh->mNormals[i];
            const aiVector3D t = mesh->mTextureCoords[0][i];
            vertices.push_back({ glm::vec3(v.x, v.y, v.z), glm::vec3(n.x, n.y, n.z), glm::vec2(t.x, 1.0f - t.y) });
        }
        for (unsigned i = 0; i != mesh->mNumFaces; i++)
        {
            for (unsigned j = 0; j != 3; j++)
                indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
        aiReleaseImport(scene);
    }

    const size_t kSizeIndices = sizeof(unsigned int) * indices.size();
    const size_t kSizeVertices = sizeof(VertexData) * vertices.size();

    GLMeshPVPPBR mesh(indices.data(), (uint32_t)kSizeIndices, (float*)vertices.data(), (uint32_t)kSizeVertices);

    GLTexture texAO(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_AO.jpg").c_str());
    GLTexture texEmissive(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_emissive.jpg").c_str());
    GLTexture texAlbedo(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_albedo.jpg").c_str());
    GLTexture texMeR(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_metalRoughness.jpg").c_str());
    GLTexture texNormal(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_normal.jpg").c_str());

    const GLuint textures[] = { texAO.getHandle(), texEmissive.getHandle(), texAlbedo.getHandle(), texMeR.getHandle(), texNormal.getHandle() };

    glBindTextures(0, sizeof(textures) / sizeof(GLuint), textures);

    // cube map
    GLTexture envMap(GL_TEXTURE_CUBE_MAP, (FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k.hdr").c_str());
    GLTexture envMapIrradiance(GL_TEXTURE_CUBE_MAP, (FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k_irradiance.hdr").c_str());
    const GLuint envMaps[] = { envMap.getHandle(), envMapIrradiance.getHandle() };
    glBindTextures(5, 2, envMaps);

    // BRDF LUT
    GLTexture brdfLUT(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "Data/brdfLUT.ktx").c_str(), GL_CLAMP_TO_EDGE);
    glBindTextureUnit(7, brdfLUT.getHandle());

    // model matrices
    const mat4 m(1.0f);
    GLBuffer modelMatrices(sizeof(mat4), glm::value_ptr(m), GL_DYNAMIC_STORAGE_BIT);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, modelMatrices.getHandle());

    positioner_->maxSpeed_ = 5.0f;

    double timeStamp = glfwGetTime();
    float deltaSeconds = 0.0f;

    FramesPerSecondCounter fpsCounter;

    while (!glfwWindowShouldClose(window_))
    {
        positioner_->update(deltaSeconds, input.mouseState->pos, input.mouseState->pressedLeft);

        const double newTimeStamp = glfwGetTime();
        deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
        timeStamp = newTimeStamp;

        fpsCounter.tick(deltaSeconds);

        // 3D Rendering Cookbook
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);
        const float ratio = width / (float)height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        const glm::mat4 p = glm::perspective(45.0f, ratio, 0.5f, 5000.0f);
        const glm::mat4 view = testCamera_->getViewMatrix();

        const PerFrameData perFrameData = { view, p, glm::vec4(testCamera_->getPosition(), 1.0f) };
        glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

        const mat4 scale = glm::scale(mat4(1.0f), vec3(5.0f));
        const mat4 rot = glm::rotate(mat4(1.0f), glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
        const mat4 pos = glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, -1.0f));
        const mat4 m = glm::rotate(scale * rot * pos, (float)glfwGetTime() * 0.1f, vec3(0.0f, 0.0f, 1.0f));
        glNamedBufferSubData(modelMatrices.getHandle(), 0, sizeof(mat4), glm::value_ptr(m));

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        program.useProgram();
        mesh.draw();

        glEnable(GL_BLEND);
        progGrid.useProgram();
        glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
        
        if(app_) app_->swapBuffers();
    }

    return 0;
}
