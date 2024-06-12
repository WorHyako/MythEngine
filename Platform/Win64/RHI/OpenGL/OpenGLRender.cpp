#include <RHI/OpenGL/OpenGLRender.hpp>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <thread>
#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>

#include "tracy/Tracy.hpp"

#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include <RHI/OpenGL/OpenGLBaseShader.hpp>
#include <Camera/BaseCamera.hpp>
#include <RHI/OpenGL/OpenGLBaseModel.hpp>
#include <Scene/VtxData.hpp>

#include <map>
#include <string_view>
#include <random>
#include <typeinfo>

#include <iostream>
#include <Filesystem/FilesystemUtilities.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <UserInput/GLFW/GLFWUserInput.hpp>

#include <Bitmap.hpp>

#include <RHI/OpenGL/Framework/UtilsGLImGui.hpp>

#define NR_POINT_LIGHTS 4
#define USE_POST_PROCESSING 0
#define USE_EXPLODE 0
#define USE_NORMALS 0

struct PerFrameData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 cameraPos;
    float tesselationScale;
    //int isWireframe;
};

struct DrawElementsIndirectCommand
{
    GLuint count_;
    GLuint instanceCount_;
    GLuint firstIndex_;
    GLuint baseVertex_;
    GLuint baseInstance_;
};

struct Texture2DParameter
{
    bool bLoadHDR;
    bool bFlipVertical;
	unsigned int Format;
    unsigned int InternalFormat;
    unsigned int TypeData;
    int WrapS;
    int WrapT;
    int MinFilter;
    int MagFilter;

    Texture2DParameter()
	    :
		bLoadHDR(false),
		bFlipVertical(true),
		Format(GL_NONE),
		InternalFormat(GL_NONE),
		TypeData(GL_UNSIGNED_BYTE),
		WrapS(GL_CLAMP_TO_EDGE),
        WrapT(GL_CLAMP_TO_EDGE),
		MinFilter(GL_LINEAR),
        MagFilter(GL_LINEAR)
    {}
    void SetWrap(int WrapMode)
    {
        WrapS = WrapMode;
        WrapT = WrapMode;
    }
} typedef Texture2DParam;

struct FMaterialStructure
{
    glm::vec3 ambientColor = glm::vec3(0.0f);
    glm::vec3 diffuseColor = glm::vec3(0.0f);
    glm::vec3 specularColor = glm::vec3(0.0f);
    glm::vec3 emissiveColor = glm::vec3(0.0f);
    // textures
    unsigned int diffuseTex = 0;
    unsigned int specularTex = 0;
    unsigned int emissiveTex = 0;
    unsigned int translucentTex = 0;
    float shininess = 32.0f;

    FMaterialStructure(unsigned int diffuseTex, unsigned int specularTex, unsigned int emissiveTex, unsigned int translucentTex, float shininess = 32.0f)
        : diffuseTex(diffuseTex)
        , specularTex(specularTex)
        , emissiveTex(emissiveTex)
        , translucentTex(translucentTex)
        , shininess(shininess)
    {}
} typedef FMaterial;

struct SpotLigthStructure
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f);
    glm::vec3 ambient = glm::vec3(0.0f);
    glm::vec3 diffuse = glm::vec3(0.0f);
    glm::vec3 specular = glm::vec3(0.0f);
    float cutOff = 0.0f;
    float outerCutOff = 0.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
} typedef SpotLight;

struct FDirectionalLightStructure
{
    glm::vec3 direction = glm::vec3(0.0f);
    glm::vec3 ambient = glm::vec3(0.0f);
    glm::vec3 diffuse = glm::vec3(0.0f);
    glm::vec3 specular = glm::vec3(0.0f);

    FDirectionalLightStructure(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
        : direction(direction)
        , ambient(ambient)
        , diffuse(diffuse)
        , specular(specular)
    {}
} typedef FDirectionalLight;

struct FPointLightStructure
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 ambient = glm::vec3(0.0f);
    glm::vec3 diffuse = glm::vec3(0.0f);
    glm::vec3 specular = glm::vec3(0.0f);
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    FPointLightStructure(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float constant = 1.0f, float linear = 0.09f, float quadratic = 0.032f)
        : position(position)
        , ambient(ambient)
        , diffuse(diffuse)
        , specular(specular)
        , constant(constant)
        , linear(linear)
        , quadratic(quadratic)
    {}
} typedef FPointLight;

struct DefaultLitModelParams
{
    // Spot light
    std::vector<SpotLight*> spotLights;

    // Point light
    std::vector<FPointLight*> pointLights;

    // Directional light
    FDirectionalLight* dirLight = nullptr;

    // coordinates
    glm::mat4 model = glm::mat4(1.0);
} typedef DefaultLitModelParams;

struct UnlitModelParams
{
    // coordinates
    glm::mat4 model = glm::mat4(1.0);
} typedef UnlitModelParams;

struct TranslucentModelParams
{
    // coordinates
    glm::mat4 model = glm::mat4(1.0);
};

struct DepthMapModelParams
{
    // coodinates
    glm::mat4 model = glm::mat4(1.0);
};

struct MeshStructure
{
    Shader* shader;
    // Material
    FMaterial* material = nullptr;
    OpenGLBaseModel* model;
    unsigned int VAO;
    unsigned int EBO;
    std::vector<glm::mat4> meshesTransforms;
    DefaultLitModelParams* defaultLitShaderParams;
    UnlitModelParams* unlitShaderParams;
    TranslucentModelParams* translucentModelParams;
    DepthMapModelParams* depthMapModelParams;
} typedef MeshStructure;

float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

void DrawOutline(MeshStructure& renderObject, glm::mat4 view, glm::mat4 projection)
{
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);
    Shader& shader = *renderObject.shader;
    shader.use();

    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    // render boxes
    glBindVertexArray(renderObject.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderObject.EBO);

    for (auto& meshTransform : renderObject.meshesTransforms)
    {
        shader.setMat4("model", meshTransform);

        if (renderObject.model != nullptr)
        {
            renderObject.model->Draw(shader);
        }
        else if (renderObject.EBO != 0)
        {
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    glEnable(GL_DEPTH_TEST);
}

enum
{
    Left = 0,
    Right = 1,
    Up = 2,
    Down = 3
}typedef pointEnum;

enum
{
    OnlyPos = 3,
    PosNormal = 6,
    PosNormalUV = 8,
    PosUV = 5
}typedef typeOfVertex;

glm::vec3 choosePointLightPosition(pointEnum index)
{
    switch (index)
    {
    case Left:
        return glm::vec3(1.0f, 0.0f, 0.0f);
    case Right:
        return glm::vec3(-1.0f, 0.0f, 0.0f);
    case Up:
        return glm::vec3(0.0f, 4.0f, -3.0f);
    case Down:
        return glm::vec3(-1.0f, 5.0f, -5.0f);
        //(-1.0f, 2.0f, -1.0f);
    default:
        return glm::vec3(0.0f);
    }
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

unsigned int loadTexture(const char* path, const Texture2DParam& TexParam);
unsigned int loadCubemap(std::vector<std::string> faces);
void renderSphere();
bool bUseBlinnPhong = false;

void RenderScene(const unsigned int framebuffer, Camera* const camera, const std::vector<MeshStructure>& defaultLitObjects,
    const std::vector<MeshStructure>& unlitObjects, const std::vector<MeshStructure>& translucentObjects, const bool bDrawStencil, const bool bUseCulling)
{
    // Render Scene
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    if (bUseCulling)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CCW);
    }

    // camera/view transformation
    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), 800.0f / 600.0f, 0.1f, 100.0f);

    for (auto& renderObject : defaultLitObjects)
    {
        const DefaultLitModelParams& shaderParams = *renderObject.defaultLitShaderParams;
        const FMaterial& material = *renderObject.material;
        // bind textures on corresponding units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.diffuseTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, material.specularTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, material.emissiveTex);

        Shader& shader = *renderObject.shader;
        shader.use();

        shader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        shader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);

        // material struct
        shader.setFloat("material.shininess", material.shininess);

        for (auto light : shaderParams.spotLights)
        {
            const SpotLight& spotLight = *light;
            // light struct
            shader.setVec3("light.position", camera->Position);
            shader.setVec3("light.direction", camera->Front);
            shader.setFloat("light.cutOff", spotLight.cutOff);
            shader.setFloat("light.outerCutOff", spotLight.outerCutOff);
            shader.setVec3("light.ambient", spotLight.ambient);
            shader.setVec3("light.diffuse", spotLight.diffuse);
            shader.setVec3("light.specular", spotLight.specular);

            shader.setFloat("light.constant", spotLight.constant);
            shader.setFloat("light.linear", spotLight.linear);
            shader.setFloat("light.quadratic", spotLight.quadratic);
        }

        unsigned int i = 0;
        for (auto light : shaderParams.pointLights)
        {
            const FPointLight& pointLight = *light;
            std::string number = std::to_string(i);
            shader.setVec3(("pointLights[" + number + "].position").c_str(), pointLight.position);
            shader.setVec3(("pointLights[" + number + "].ambient").c_str(), pointLight.ambient);
            shader.setVec3(("pointLights[" + number + "].diffuse").c_str(), pointLight.diffuse);
            shader.setVec3(("pointLights[" + number + "].specular").c_str(), pointLight.specular);
            shader.setFloat(("pointLights[" + number + "].constant").c_str(), pointLight.constant);
            shader.setFloat(("pointLights[" + number + "].linear").c_str(), pointLight.linear);
            shader.setFloat(("pointLights[" + number + "].quadratic").c_str(), pointLight.quadratic);
            i++;
        }

        const FDirectionalLight& dirLight = *shaderParams.dirLight;

        shader.setVec3("dirLight.direction", dirLight.direction);
        shader.setVec3("dirLight.ambient", dirLight.ambient);
        shader.setVec3("dirLight.diffuse", dirLight.diffuse);
        shader.setVec3("dirLight.specular", dirLight.specular);

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        // render boxes
        glBindVertexArray(renderObject.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderObject.EBO);

        for (auto& meshTransform : renderObject.meshesTransforms)
        {
            renderObject.shader->setMat4("model", meshTransform);

            if (renderObject.model != nullptr)
            {
                renderObject.model->Draw(shader);
            }
            else if (renderObject.EBO != 0)
            {
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            }
            else
            {
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
    }

    for (auto& renderObject : unlitObjects)
    {
        Shader& shader = *renderObject.shader;
        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        // material struct
        const FMaterial& material = *renderObject.material;
        shader.setVec3("material.ambient", material.ambientColor);

        // render boxes
        glBindVertexArray(renderObject.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderObject.EBO);

        for (auto meshTransform : renderObject.meshesTransforms)
        {
            shader.setMat4("model", meshTransform);

            // draw the light cube object
            if (renderObject.model != nullptr)
            {
                renderObject.model->Draw(shader);
            }
            else if (renderObject.EBO != 0)
            {
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            }
            else
            {
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
    }

    std::map<float, glm::mat4> sorted;
    std::vector<MeshStructure> allMeshes{ translucentObjects };
    //allMeshes.insert(std::begin(allMeshes), std::begin(defaultLitObjects), std::end(defaultLitObjects));
    //allMeshes.insert(std::begin(allMeshes), std::begin(unlitObjects), std::end(unlitObjects));

    for (unsigned int i = 0; i < allMeshes.size(); i++)
    {
        for (auto& meshTransform : allMeshes[i].meshesTransforms)
        {
            float distance = glm::length(camera->Position - glm::vec3(meshTransform[3].x, meshTransform[3].y, meshTransform[3].z));
            sorted[distance] = meshTransform;
        }
    }

    for (auto& renderObject : translucentObjects)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderObject.material->translucentTex);

        Shader& shader = *renderObject.shader;
        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        glBindVertexArray(renderObject.VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderObject.EBO);

        for (std::map<float, glm::mat4>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            shader.setMat4("model", it->second);

            // draw the light cube object
            if (renderObject.model != nullptr)
            {
                renderObject.model->Draw(shader);
            }
            else if (renderObject.EBO != 0)
            {
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
            else
            {
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }
    }

    if (bUseCulling)
    {
        glDisable(GL_CULL_FACE);
    }

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
}

void RenderDepthMapScene(const unsigned int framebuffer, std::vector<MeshStructure>& depthMapObjects, Shader& depthMapShader, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightPos, const bool bUseCulling, const std::vector<glm::mat4> shadowTransforms)
{
    // Render Scene
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    if (bUseCulling)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CCW);
    }

    Shader& shader = depthMapShader;
    shader.use();

    for (auto& renderObject : depthMapObjects)
    {
        shader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        for (unsigned int i = 0; i < 6; ++i)
            shader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);

        shader.setFloat("far_plane", 25.0f);
        shader.setVec3("lightPos", lightPos);

        // render boxes
        glBindVertexArray(renderObject.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderObject.EBO);

        for (auto& meshTransform : renderObject.meshesTransforms)
        {
            shader.setMat4("model", meshTransform);

            if (renderObject.model != nullptr)
            {
                renderObject.model->Draw(shader);
            }
            else if (renderObject.EBO != 0)
            {
                glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            }
            else
            {
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
    }

    if (bUseCulling)
    {
        glDisable(GL_CULL_FACE);
    }
}

unsigned int TBNQuadVAO, TBNQuadVBO;
void renderTBNQuad()
{
    if (TBNQuadVAO == 0)
    {
        // vertex coordinates
        glm::vec3 pos1(-1.0f, 1.0f, 0.0f);
        glm::vec3 pos2(-1.0f, -1.0f, 0.0f);
        glm::vec3 pos3(1.0f, -1.0f, 0.0f);
        glm::vec3 pos4(1.0f, 1.0f, 0.0f);
        /*glm::vec3 pos1(-1.0f, 0.0f, 1.0f);
        glm::vec3 pos2(-1.0f, 0.0f, -1.0f);
        glm::vec3 pos3(1.0f, 0.0f, -1.0f);
        glm::vec3 pos4(1.0f, 0.0f, 1.0f);*/
        // texture coordinates
        glm::vec2 uv1(0.0f, 1.0f);
        glm::vec2 uv2(0.0f, 0.0f);
        glm::vec2 uv3(1.0f, 0.0f);
        glm::vec2 uv4(1.0f, 1.0f);
        // normal vector
        glm::vec3 nm(0.0f, 0.0f, 1.0f);

        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        glm::vec3 tangent1(0.0f);
        float f = 1 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent1 = glm::normalize(tangent1);

        /*glm::vec3 bitangent1(0.0f);
        bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent1 = glm::normalize(bitangent1*/

        edge1 = pos3 - pos1;
        edge2 = pos4 - pos1;
        deltaUV1 = uv3 - uv1;
        deltaUV2 = uv4 - uv1;

        glm::vec3 tangent2(0.0f);
        f = 1 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent2 = glm::normalize(tangent2);

        /*glm::vec3 bitangent2(0.0f);
        bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent2 = glm::normalize(bitangent2);*/

        float TBNQuad[] = {
            // positions            // normal         // texcoords  // tangent                          
                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z,
                pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z,

                pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z,
                pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z,
                pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z
        };

        glGenVertexArrays(1, &TBNQuadVAO);
        glGenBuffers(1, &TBNQuadVBO);
        glBindVertexArray(TBNQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, TBNQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(TBNQuad), &TBNQuad, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    glBindVertexArray(TBNQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void createTextureAttachment(unsigned int& texture, const unsigned int width, const unsigned int height)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glBindTexture(GL_TEXTURE_2D, 0);

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
}

void createTextureMultisampleAttachment(unsigned int& texture, const float sample, const unsigned int width, const unsigned int height)
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);

    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sample, GL_RGB, width, height, GL_TRUE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
}

void createRenderBufferObject(unsigned int& rbo, const unsigned int width, const unsigned int height)
{
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    // use a single renderbuffer object for both a depth AND stencil buffer
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    // now actually attach it
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void createRenderBufferMultisampleObject(unsigned int& rbo, const unsigned int width, const unsigned int height)
{
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);

    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void createBuffer(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO, typeOfVertex typeVertex, std::vector<float> vertices, unsigned int sizeOfVertex, std::vector<unsigned int> indices = std::vector<unsigned int>())
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    if (indices.size() > 0)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);
    }

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (int)sizeOfVertex * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    if (typeVertex == typeOfVertex::PosNormal || typeVertex == typeOfVertex::PosNormalUV)
    {
        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (int)sizeOfVertex * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    if (typeVertex == typeOfVertex::PosNormalUV)
    {
        // texture coord attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (int)sizeOfVertex * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    else if (typeVertex == typeOfVertex::PosUV)
    {
        // texture coord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (int)sizeOfVertex * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //if (sizeof(indices) != 0)
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    //glBindVertexArray(0);*/
}

void BindBuffer(unsigned int& buffer)
{
    std::vector<unsigned int> data = {};
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(unsigned int), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 10, data.size() * sizeof(unsigned int), data.data());

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    // get pointer
    void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    // now copy data into memory
    memcpy(ptr, data.data(), sizeof(data));
    // make sure to tell OpenGL we're done the pointer
    glUnmapBuffer(GL_ARRAY_BUFFER);

    float positions[] = { 1.0f };
    float normals[] = { 1.0f };
    float texs[] = { 1.0f };
    // fill buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), &positions);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(normals), &normals);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(normals), sizeof(texs), &texs);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(sizeof(positions)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(sizeof(positions) + sizeof(normals)));

    unsigned int vbo1, vbo2;
    glGenBuffers(1, &vbo1);
    glGenBuffers(2, &vbo2);
    glBindBuffer(GL_COPY_READ_BUFFER, vbo1);
    // another approach
    //glBindBuffer(GL_ARRAY_BUFFER, vbo1);
    glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 8 * sizeof(float));
}

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;

const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// camera
//Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f; // time at last frame

// light
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec2;

OpenGLRender::OpenGLRender(GLApp* app)
	: OpenGLBaseRender(app)
{}

int OpenGLRender::draw()
{
    // configure global opengl state
    // ------------------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_MULTISAMPLE);

    // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    Camera& camera = Camera::GetCamera();
    GLFWUserInput& input = GLFWUserInput::GetInput();

    input.lastX = SCR_WIDTH / 2;
    input.lastY = SCR_HEIGHT / 2;

    // gamma - correction
    //glEnable(GL_FRAMEBUFFER_SRGB);

    // build and compile our shader program
    // ------------------------------------
	Shader cubeShader("SimpleCubeShader/cubeShader.vert", "SimpleCubeShader/cubeShader.frag");
    Shader lightingSourceShader("SimpleLightShader/lightshader.vert", "SimpleLightShader/lightshader.frag");
    Shader modelShader(
        "ModelShader/modelShader.vert",
        "ModelShader/modelShader.frag"
#if USE_EXPLODE == 1
        , "ModelShader/ExplodeGeometryShader.geom"
#endif
    );
    Shader simpleModelShader(
        "ModelShader/SimpleModelShader.vert",
        "ModelShader/SimpleModelShader.frag"
    );
    Shader normalModelShader(
        "NormalModelShader/NormalModelShader.vert",
        "NormalModelShader/NormalModelShader.frag",
        "NormalModelShader/NormalModelShader.geom"
    );
    Shader stencilShader("SimpleCubeShader/cubeShader.vert", "SimpleStencilShader/stencilShader.frag");
    Shader vegetationShader("SimpleVegetationShader/vegetationShader.vert", "SimpleVegetationShader/vegetationShader.frag");
    Shader cullingShader("SimpleCullingShader/cullingShader.vert", "SimpleCullingShader/cullingShader.frag");
    Shader screenShader("SimpleQuadShader/quadShader.vert", "SimpleQuadShader/quadShader.frag");
    Shader skyboxShader("SimpleSkyboxShader/skybox.vert", "SimpleSkyboxShader/skybox.frag");
    Shader reflectionShader("SimpleReflectionShader/reflection.vert", "SimpleReflectionShader/reflection.frag");
    Shader refractionShader("SimpleRefractionShader/refraction.vert", "SimpleRefractionShader/refraction.frag");
    Shader GeometryHouseShader(
        "SimpleHouseGeometryShader/GeometryHousesShader.vert",
        "SimpleHouseGeometryShader/GeometryHousesShader.frag",
        "SimpleHouseGeometryShader/GeometryHousesShader.geom");
    Shader simpleInstanceShader(
        "SimpleInstancedShader/SimpleInstancedShader.vert",
        "SimpleInstancedShader/SimpleInstancedShader.frag");
    Shader modelInstanceShader(
        "ModelInstancedShader/ModelInstancedShader.vert",
        "ModelInstancedShader/ModelInstancedShader.frag");
    /*Shader depthMapShader(
        "DepthMapShaders/DepthMapShader.vert",
        "DepthMapShaders/DepthMapShader.frag");*/
    Shader depthMapDebugScreenShader(
        "DepthMapShaders/DepthMapDebugScreenShader.vert",
        "DepthMapShaders/DepthMapDebugScreenShader.frag");
    Shader gaussianBlurShader(
        "BloomShader/GaussianBlur.vert",
        "BloomShader/GaussianBlur.frag");
    Shader hdrScreenShader(
        "HDRScreenShader/HDRScreenShader.vert",
        "HDRScreenShader/HDRScreenShader.frag");
    Shader gBufferGeometryShader(
        "GBufferShader/GBufferGeometryShader.vert",
        "GBufferShader/GBufferGeometryShader.frag");
    Shader ssaoShader(
        "SSAO/SSAO.vert",
        "SSAO/SSAO.frag");
    Shader ssaoBlurShader(
        "SSAO/SSAOBlur.vert",
        "SSAO/SSAOBlur.frag");
    Shader lightingPassShader(
        "GBufferShader/LightingPassShader.vert",
        "GBufferShader/LightingPassShader.frag");
    Shader lightSourceShader(
        "LightSource/LightSource.vert",
        "LightSource/LightSource.frag");
    Shader depthCubemapShader(
        "DepthMapShaders/DepthMapShader.vert",
        "DepthMapShaders/DepthCubemapShader.frag",
        "DepthMapShaders/DepthCubemapShader.geom");
    Shader normalMappingShaderToWorld(
        "normalMappingShader/normalMappingShaderToWorld.vert",
        "normalMappingShader/normalMappingShaderToWorld.frag");
    Shader normalMappingShaderToLocal(
        "normalMappingShader/normalMappingShaderToLocal.vert",
        "normalMappingShader/normalMappingShaderToLocal.frag");
    Shader parallaxMappingShader(
        "ParallaxMappingShader/ParallaxMappingShader.vert",
        "ParallaxMappingShader/ParallaxMappingShader.frag");
    Shader pbrShader(
        "PBRShader/PBR.vert",
        "PBRShader/PBR.frag");
    Shader equirectangularToCubemapShader(
        "IBLDiffuseShader/Cubemap.vert",
        "IBLDiffuseShader/EquirectangularToCubemap.frag");
    Shader irradianceShader(
        "IBLDiffuseShader/Cubemap.vert",
        "IBLDiffuseShader/IrradianceConvulution.frag");
    Shader prefilterShader(
        "IBLDiffuseShader/Cubemap.vert",
        "IBLSpecularShader/Prefilter.frag");
    Shader brdfShader(
        "IBLSpecularShader/brdf.vert",
        "IBLSpecularShader/brdf.frag");
    Shader backgroundIBLShader(
        "IBLDiffuseShader/Background.vert",
        "IBLDiffuseShader/Background.frag");

    stbi_set_flip_vertically_on_load(true);
    // load models
    OpenGLBaseModel backpackModel(FilesystemUtilities::GetResourcesDir() + "objects/backpack/backpack.obj");
    OpenGLBaseModel rockModel(FilesystemUtilities::GetResourcesDir() + "objects/rock/rock.obj");
    OpenGLBaseModel planetModel(FilesystemUtilities::GetResourcesDir() + "objects/planet/planet.obj");
    OpenGLBaseModel gunModel(FilesystemUtilities::GetResourcesDir() + "objects/Cerberus_by_Andrew_Maximov/Cerberus_LP.FBX");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    /*float vertices[] = {
        // positions         // colors          // texture coords
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,  // top right near
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  // bottom right near
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,  // bottom left near
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  0.0f, 1.0f,  // top left near
    };*/
    std::vector<float> vertices = {
        // positions        // normals              // textures    
    -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,     0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,    -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,    -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,    -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,    -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,    -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,    -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,     1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,     1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,     0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,     0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,     0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,     1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,     0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,     1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,     1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,     1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,     0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,     0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,     0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,     1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,     1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,     1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,     0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,     0.0f, 1.0f
    };
    std::vector<float> verticesVegetations =
    {
        -0.5f, -0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,    0.0f,  0.0f, 1.0f,      0.0f, 1.0f,
    };
    std::vector<unsigned int> indicesVegetations =
    {
        0, 1, 3,
        1, 2, 3
    };
    unsigned int vegetationsVAO, vegetationsVBO, vegetationsEBO;
    createBuffer(vegetationsVAO, vegetationsVBO, vegetationsEBO, typeOfVertex::PosNormalUV, verticesVegetations, 8, indicesVegetations);

    unsigned int cubeVAO, cubeVBO, cubeEBO;
    createBuffer(cubeVAO, cubeVBO, cubeEBO, typeOfVertex::PosNormalUV, vertices, 8);

    unsigned int reflectionVAO, reflectionVBO, reflectionEBO;
    createBuffer(reflectionVAO, reflectionVBO, reflectionEBO, typeOfVertex::PosNormal, vertices, 8);

    unsigned int refractionVAO, refractionVBO, refractionEBO;
    createBuffer(refractionVAO, refractionVBO, refractionEBO, typeOfVertex::PosNormal, vertices, 8);

    std::vector<glm::vec3> positionVegetations =
    {
        glm::vec3(2.0f, 0.0f, 2.0f),
        glm::vec3(2.0f, 0.0f, -2.0f),
        glm::vec3(-2.0f, 0.0f, 2.0f),
        glm::vec3(-2.0f, 0.0f, -2.0f),
        glm::vec3(-3.0f, 0.0f, -3.0f),
        glm::vec3(3.0f, 0.0f, 3.0f)
    };
    glm::vec3 positions[] =
    {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3,  // second Triangle
    };

    glm::vec3 pointLightPositions[] =
    {
        glm::vec3(0.7f, 0.2f, 2.0f),
        glm::vec3(2.3f, -3.3f, 4.0f),
        glm::vec3(-4.0f, 2.0f, -12.0f),
        glm::vec3(0.0f, 0.0f, -3.0f)
    };
    unsigned int VBO, VAO, EBO;
    createBuffer(VAO, VBO, EBO, typeOfVertex::PosNormalUV, vertices, 8);

    std::vector<float> cullingVerticesCW = {
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right         
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
        // Left face
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-left
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
        // Right face
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right         
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left     
         // Bottom face
         -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
          0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // top-left
          0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
          0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
         -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
         -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
         // Top face
         -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
          0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
          0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right     
          0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
         -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
         -0.5f,  0.5f,  0.5f,  0.0f, 0.0f  // bottom-left        
    };
    std::vector<float> cullingVerticesCCW = {
        // Back face
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // Bottom-left
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // bottom-right    
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right              
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // bottom-left                
    // Front face
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-right        
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // top-right
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // top-left        
    // Left face
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-left       
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-right
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
    // Right face
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right      
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right          
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // bottom-right
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-left
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // top-left
     // Bottom face          
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
      0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // top-left        
      0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // bottom-left
     -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // top-right
     -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // bottom-right
     // Top face
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // top-left
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // top-right
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right                 
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // bottom-right
     -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, // bottom-left  
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f  // top-left
    };
    unsigned int cullingVBO, cullingVAO, cullingEBO;
    createBuffer(cullingVAO, cullingVBO, cullingEBO, typeOfVertex::PosUV, cullingVerticesCCW, 5);

    std::vector<float> lightVertices = {
        // positions         // colors
         0.5f,  0.5f,  -0.5f,  1.0f, 0.0f, 0.0f,// top right near
         0.5f, -0.5f,  -0.5f,  0.0f, 1.0f, 0.0f,// bottom right near
        -0.5f, -0.5f,  -0.5f,  0.0f, 0.0f, 1.0f,// bottom left near
        -0.5f,  0.5f,  -0.5f,  0.5f, 0.5f, 0.5f,// top left near
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,// top right near
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,// bottom right near
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,// bottom left near
        -0.5f,  0.5f,  0.5f,  0.5f, 0.5f, 0.5f,// top left near
    };
    std::vector<unsigned int> lightIndices = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3,  // second Triangle
        0, 1, 4,
        1, 4, 5,
        0, 3, 4,
        3, 4, 7,
        2, 3, 6,
        3, 6, 7,
        4, 5, 6,
        4, 6, 7,
        1, 2, 5,
        2, 5, 6
    };

    unsigned int lightVAO, lightVBO, lightEBO;
    createBuffer(lightVAO, lightVBO, lightEBO, typeOfVertex::PosNormal, lightVertices, 6, lightIndices);

    std::vector<float> skyboxVertices = {
        // positions         // colors
         1.0f,  1.0f,  -1.0f, // top right near
         1.0f, -1.0f,  -1.0f, // bottom right near
        -1.0f, -1.0f,  -1.0f, // bottom left near
        -1.0f,  1.0f,  -1.0f, // top left near
         1.0f,  1.0f,  1.0f,  // top right far
         1.0f, -1.0f,  1.0f,  // bottom right far
        -1.0f, -1.0f,  1.0f,  // bottom left far
        -1.0f,  1.0f,  1.0f,  // top left far
    };
    std::vector<unsigned int> skyboxIndices = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3,  // second Triangle
        0, 1, 4,
        1, 4, 5,
        0, 3, 4,
        3, 4, 7,
        2, 3, 6,
        3, 6, 7,
        4, 5, 6,
        4, 6, 7,
        1, 2, 5,
        2, 5, 6
    };

    unsigned int skyboxVAO, skyboxVBO, skyboxEBO;
    createBuffer(skyboxVAO, skyboxVBO, skyboxEBO, typeOfVertex::OnlyPos, skyboxVertices, 3, skyboxIndices);

    float GeometryHousesVertexInfo[] = {
        -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,// top - left
         0.5f, 0.5f, 0.0f, 1.0f, 0.0f,// top - right
        0.5f, -0.5f, 0.0f, 0.0f, 1.0f,// bottom - left 
        -0.5f, -0.5f, 1.0f, 0.0f, 1.0f// bottom - right
    };

    unsigned int GeometryHousesVAO, GeometryHousesVBO;
    glGenVertexArrays(1, &GeometryHousesVAO);
    glGenBuffers(1, &GeometryHousesVBO);

    glBindVertexArray(GeometryHousesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, GeometryHousesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GeometryHousesVertexInfo), &GeometryHousesVertexInfo, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(unsigned int), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(unsigned int), (void*)(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glm::vec2 translations[100];
    int index = 0;
    float offset = 0.1f;
    for (int y = -10; y < 10; y += 2)
    {
        for (int x = -10; x < 10; x += 2)
        {
            glm::vec2 translation;
            translation.x = (float)x / 10.0f + offset;
            translation.y = (float)y / 10.0f + offset;
            translations[index++] = translation;
        }
    }

    unsigned int instancedVBO;
    glGenBuffers(1, &instancedVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instancedVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 100, &translations[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    float quadInstancedVertices[] = {
        // positions   // colors
        -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
        -0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
         0.05f, -0.05f,  0.0f, 0.0f, 1.0f,

        -0.05f,  0.05f,  1.0f, 0.0f, 0.0f,
         0.05f, -0.05f,  0.0f, 1.0f, 0.0f,
         0.05f,  0.05f,  0.0f, 0.0f, 1.0f
    };
    unsigned int quadInstancedVAO, quadInstancedVBO;
    glGenVertexArrays(1, &quadInstancedVAO);
    glGenBuffers(1, &quadInstancedVBO);
    glBindVertexArray(quadInstancedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadInstancedVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadInstancedVertices), &quadInstancedVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, instancedVBO);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, instancedVBO);
    glVertexAttribDivisor(2, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    unsigned int amount = 10000;
    glm::mat4* modelMatrices;
    modelMatrices = new glm::mat4[amount];
    srand(glfwGetTime()); // initialize random seed
    float radius = 25.0f;
    offset = 10.0f;
    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / float(amount) * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius * displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius * displacement;
        model = glm::translate(model, glm::vec3(x, y - 10.0f, z));

        // 2. scale: scale between 0.05f and 0.25f
        float scale = static_cast<float>((rand() % 20) / 100.0f + 0.05f);
        model = glm::scale(model, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = static_cast<float>((rand() % 360));
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        modelMatrices[i] = model;
    }

    // vertex buffer object
    unsigned int instanceBuffer;
    glGenBuffers(1, &instanceBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer);
    glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    std::vector<const OpenGLBaseMesh*> rockMeshes;
    for(auto& rockMesh : rockModel.GetMeshes())
    {
        const OpenGLBaseMesh* mesh = reinterpret_cast<const OpenGLBaseMesh*>(rockMesh);
        rockMeshes.push_back(mesh);
    }

    for (unsigned int i = 0; i < rockMeshes.size(); i++)
    {
        unsigned int VAO = rockMeshes[i]->VAO;
        glBindVertexArray(VAO);
        // vertex attributes
        std::size_t vec4Size = sizeof(glm::vec4);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -0.25f,  1.0f,  0.0f, 1.0f,
        -0.25f,  0.5f,  0.0f, 0.0f,
         0.25f,  0.5f,  1.0f, 0.0f,

        -0.25f,  1.0f,  0.0f, 1.0f,
         0.25f,  0.5f,  1.0f, 0.0f,
         0.25f,  1.0f,  1.0f, 1.0f
    };
    unsigned int screenVAO, screenVBO;
    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);
    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    float fullScreenQuadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.f,  -1.0f,  1.0f, 0.0f,

         1.0f,  -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
         -1.0f, 1.0f,  0.0f, 1.0f
    };
    unsigned int fullScreenVAO, fullScreenVBO;
    glGenVertexArrays(1, &fullScreenVAO);
    glGenBuffers(1, &fullScreenVBO);
    glBindVertexArray(fullScreenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fullScreenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fullScreenQuadVertices), &fullScreenQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // depth map framebuffer
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // cube depth map framebuffer
    unsigned int depthCubemapFBO;
    glGenFramebuffers(1, &depthCubemapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthCubemapFBO);

    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // HDR framebuffer
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int colorTextures[2];
    glGenTextures(2, colorTextures);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorTextures[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorTextures[i], 0);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    unsigned int hdrRBO;
    createRenderBufferObject(hdrRBO, SCR_WIDTH, SCR_HEIGHT);

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // GaussianBlur framebuffer
    unsigned int pingpongFBO[2];
    unsigned int pingpongBuffer[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0);

        // finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // gBuffer
    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;
    // View position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int gBufferAttachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, gBufferAttachments);

    unsigned int gBufferRBO;
    glGenRenderbuffers(1, &gBufferRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, gBufferRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gBufferRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<glm::vec3> objectPositions;
    objectPositions.push_back(glm::vec3(-3.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3(0.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3(3.0, -0.5, -3.0));
    objectPositions.push_back(glm::vec3(-3.0, -0.5, 0.0));
    objectPositions.push_back(glm::vec3(0.0, -0.5, 0.0));
    objectPositions.push_back(glm::vec3(3.0, -0.5, 0.0));
    objectPositions.push_back(glm::vec3(-3.0, -0.5, 3.0));
    objectPositions.push_back(glm::vec3(0.0, -0.5, 3.0));
    objectPositions.push_back(glm::vec3(3.0, -0.5, 3.0));

    // lightning info
    // --------------
    const unsigned int NR_LIGHTS = 32;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;
    srand(13);
    for (unsigned int i = 0; i < NR_LIGHTS; i++)
    {
        // calculate slightly random offsets
        float xPos = static_cast<float>(((rand() % 100) / 100.0f) * 6.0f - 3.0f);
        float yPos = static_cast<float>(((rand() % 100) / 100.0f) * 6.0f - 4.0f);
        float zPos = static_cast<float>(((rand() % 100) / 100.0f) * 6.0f - 3.0f);
        lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
        // also calculate random color
        float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5f); // between 0.5f and 1.0f
        float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5f); // between 0.5f and 1.0f
        float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5f); // between 0.5f and 1.0f
        lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }

    // SSAO hemisphere
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;
    std::vector<glm::vec3> ssaoKernel;
    for (unsigned int i = 0; i < 64; i++)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0f;
        scale = lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // SSAO noise
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f);
        ssaoNoise.push_back(noise);
    }

    unsigned int noiseTexture;
    glGenTextures(1, &noiseTexture);
    glBindTexture(GL_TEXTURE_2D, noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    // SSAO FBO
    unsigned int ssaoFBO;
    glGenFramebuffers(1, &ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    unsigned int ssaoColorBuffer;
    glGenTextures(1, &ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;

    // SSAO Blur FBO
    unsigned int ssaoBlurFBO;
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);

    unsigned int ssaoColorBufferBlur;
    glGenTextures(1, &ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Blur Framebuffer not complete!" << std::endl;

    // IBL Diffuse
    // Framebuffer
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Textures
    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // framebuffer configuration
    // -------------------------
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    // create a color attachment texture
    unsigned int textureColorbuffer;
    createTextureAttachment(textureColorbuffer, SCR_WIDTH, SCR_HEIGHT);

    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    unsigned int rbo;
    createRenderBufferObject(rbo, SCR_WIDTH, SCR_HEIGHT);

    //GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    //glDrawBuffers(1, DrawBuffers);

    if (glCheckFramebufferStatus(framebuffer) == GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glDeleteFramebuffers(1, &framebuffer);

    // multisample framebuffer
    unsigned int msFBO;
    glGenFramebuffers(1, &msFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, msFBO);
    unsigned int msFBOColorAttachment;
    createTextureMultisampleAttachment(msFBOColorAttachment, 2.0f, SCR_WIDTH, SCR_HEIGHT);

    unsigned int msFBORenderAttachment;
    createRenderBufferMultisampleObject(msFBORenderAttachment, SCR_WIDTH, SCR_HEIGHT);

    if (glCheckFramebufferStatus(msFBO) == GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int uniformBufferCubeIndex = glGetUniformBlockIndex(cubeShader.ID, "Matrices");
    unsigned int uniformBufferNormalMappingToWorldIndex = glGetUniformBlockIndex(normalMappingShaderToWorld.ID, "Matrices");
    unsigned int uniformBufferNormalMappingToLocalIndex = glGetUniformBlockIndex(normalMappingShaderToLocal.ID, "Matrices");
    unsigned int uniformBufferParallaxMappingIndex = glGetUniformBlockIndex(parallaxMappingShader.ID, "Matrices");
    unsigned int uniformBufferGBufferGeometryShaderIndex = glGetUniformBlockIndex(gBufferGeometryShader.ID, "Matrices");
    unsigned int uniformBufferGBufferLightSourceIndex = glGetUniformBlockIndex(lightSourceShader.ID, "Matrices");
    unsigned int uniformBufferPBRShaderIndex = glGetUniformBlockIndex(pbrShader.ID, "Matrices");
    unsigned int uniformBufferBackgroundIBLShaderIndex = glGetUniformBlockIndex(backgroundIBLShader.ID, "Matrices");

    glUniformBlockBinding(cubeShader.ID, uniformBufferCubeIndex, 0);
    glUniformBlockBinding(normalMappingShaderToWorld.ID, uniformBufferNormalMappingToWorldIndex, 0);
    glUniformBlockBinding(normalMappingShaderToLocal.ID, uniformBufferNormalMappingToLocalIndex, 0);
    glUniformBlockBinding(parallaxMappingShader.ID, uniformBufferParallaxMappingIndex, 0);
    glUniformBlockBinding(gBufferGeometryShader.ID, uniformBufferGBufferGeometryShaderIndex, 0);
    glUniformBlockBinding(lightSourceShader.ID, uniformBufferGBufferLightSourceIndex, 0);
    glUniformBlockBinding(pbrShader.ID, uniformBufferPBRShaderIndex, 0);
    glUniformBlockBinding(backgroundIBLShader.ID, uniformBufferBackgroundIBLShaderIndex, 0);

    unsigned int uboMatrices;
    glGenBuffers(1, &uboMatrices);

    glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;
    int maxVertexUniformComponents;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniformComponents);
    std::cout << "Maximum of vertex uniform components supported: " << maxVertexUniformComponents << std::endl;

    // load and create a texture (we now use a utility function to keep the code more organized)
    // -----------------------------------------------------------------------------------------
    // diffuseMap
    // ----------
    // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
    Texture2DParam TexParam;
    TexParam.MinFilter = GL_LINEAR_MIPMAP_LINEAR;
    unsigned int diffuseMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/container2.png").c_str()/*, GL_SRGB */, TexParam);
    // specularMap
    // -----------
    unsigned int specularMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/container2_specular.png").c_str(), TexParam);
    // emissionMap
    // -----------
    unsigned int emissionMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/matrix.jpg").c_str(), TexParam);

    unsigned int translucencyMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/blending_transparent_window.png").c_str(), TexParam);

    // PBR maps
    Texture2DParam PBR_TexParam;
    PBR_TexParam.MinFilter = GL_LINEAR_MIPMAP_LINEAR;
    PBR_TexParam.SetWrap(GL_REPEAT);
    PBR_TexParam.bFlipVertical = false;
    //unsigned int PBR_AlbedoMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/PBR/rustediron2_basecolor.png").c_str(), PBR_TexParam);
    //unsigned int PBR_NormalMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/PBR/rustediron2_normal.png").c_str(), PBR_TexParam);
    //unsigned int PBR_MetallicMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/PBR/rustediron2_metallic.png").c_str(), PBR_TexParam);
    //unsigned int PBR_RoughnessMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/PBR/rustediron2_roughness.png").c_str(), PBR_TexParam);
    //unsigned int PBR_AOMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/PBR/rustediron2_roughness.png").c_str(), PBR_TexParam);
    //unsigned int PBR_AlbedoMap = loadTexture(FileSystem::getPath("resources/textures/IBL_PBR/rusted-steel-bl/rusted-steel_albedo.png").c_str(), GL_NONE, GL_NONE, GL_UNSIGNED_BYTE, GL_REPEAT);
    //unsigned int PBR_NormalMap = loadTexture(FileSystem::getPath("resources/textures/IBL_PBR/rusted-steel-bl/rusted-steel_normal-ogl.png").c_str(), GL_NONE, GL_NONE, GL_UNSIGNED_BYTE, GL_REPEAT);
    //unsigned int PBR_MetallicMap = loadTexture(FileSystem::getPath("resources/textures/IBL_PBR/rusted-steel-bl/rusted-steel_metallic.png").c_str(), GL_NONE, GL_NONE, GL_UNSIGNED_BYTE, GL_REPEAT);
    //unsigned int PBR_RoughnessMap = loadTexture(FileSystem::getPath("resources/textures/IBL_PBR/rusted-steel-bl/rusted-steel_roughness.png").c_str(), GL_NONE, GL_NONE, GL_UNSIGNED_BYTE, GL_REPEAT);
    //unsigned int PBR_AOMap = loadTexture(FileSystem::getPath("resources/textures/IBL_PBR/rusted-steel-bl/rusted-steel_ao.png").c_str(), GL_NONE, GL_NONE, GL_UNSIGNED_BYTE, GL_REPEAT);
    unsigned int PBR_AlbedoMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "objects/Cerberus_by_Andrew_Maximov/Textures/Cerberus_A.tga").c_str(), PBR_TexParam);
    unsigned int PBR_NormalMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "objects/Cerberus_by_Andrew_Maximov/Textures/Cerberus_N.tga").c_str(), PBR_TexParam);
    unsigned int PBR_MetallicMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "objects/Cerberus_by_Andrew_Maximov/Textures/Cerberus_M.tga").c_str(), PBR_TexParam);
    unsigned int PBR_RoughnessMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "objects/Cerberus_by_Andrew_Maximov/Textures/Cerberus_R.tga").c_str(), PBR_TexParam);
    unsigned int PBR_AOMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "objects/Cerberus_by_Andrew_Maximov/Textures/Raw/Cerberus_AO.tga").c_str(), PBR_TexParam);

    //unsigned int EquirectangularMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/IBLDiffuse/newport_loft.hdr").c_str(), GL_RGB16F, GL_RGB, GL_FLOAT, GL_CLAMP_TO_EDGE, GL_LINEAR);
    Texture2DParam EquirectangularTexParam;
    EquirectangularTexParam.bLoadHDR = true;
    EquirectangularTexParam.InternalFormat = GL_RGB16F;
    EquirectangularTexParam.Format = GL_RGB;
    EquirectangularTexParam.TypeData = GL_FLOAT;
    unsigned int EquirectangularMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/IBLDiffuse/newport_loft.hdr").c_str(), EquirectangularTexParam);

    // PBR Shader
    pbrShader.use();
    pbrShader.setVec3("albedo", glm::vec3(0.5f, 0.0f, 0.0f));
    pbrShader.setFloat("AO", 1.0f);

    // lights
    // ------
    glm::vec3 pbrLightPositions[] = {
        glm::vec3(-10.0f,  10.0f, 10.0f),
        glm::vec3(10.0f,  10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f),
        glm::vec3(10.0f, -10.0f, 10.0f),
    };
    glm::vec3 pbrLightColors[] = {
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f)
    };
    int nrRows = 7;
    int nrColumns = 7;
    float spacing = 2.5f;

    std::vector<std::string> faces_path =
    {
        FilesystemUtilities::GetResourcesDir() + "textures/skybox/right.jpg",
        FilesystemUtilities::GetResourcesDir() + "textures/skybox/left.jpg",
        FilesystemUtilities::GetResourcesDir() + "textures/skybox/top.jpg",
        FilesystemUtilities::GetResourcesDir() + "textures/skybox/bottom.jpg",
        FilesystemUtilities::GetResourcesDir() + "textures/skybox/front.jpg",
        FilesystemUtilities::GetResourcesDir() + "textures/skybox/back.jpg"
    };
    unsigned int textureCubemap = loadCubemap(faces_path);

    //GLint internalFormat = GL_RGBA, GLenum format = GL_NONE, GLenum typeData = GL_UNSIGNED_BYTE, GLint textureWrapParam = GL_CLAMP_TO_EDGE, GLint minFilter = GL_LINEAR_MIPMAP_LINEAR, GLint maxFilter = GL_LINEAR
    Texture2DParam MappingTexParam;
    MappingTexParam.InternalFormat = GL_RGB;
    MappingTexParam.SetWrap(GL_REPEAT);
    unsigned int normalMappingDiffuseMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/NormalMapping/brickwall.jpg").c_str(), MappingTexParam);
    unsigned int normalMappingNormalMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/NormalMapping/brickwall_normal.jpg").c_str(), MappingTexParam);

    unsigned int parallaxMappingDiffuseMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/ParallaxMapping/bricks_diffuse.jpg").c_str(), MappingTexParam);
    unsigned int parallaxMappingNormalMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/ParallaxMapping/bricks_normal.jpg").c_str(), MappingTexParam);
    unsigned int parallaxMappingDepthMap = loadTexture((FilesystemUtilities::GetResourcesDir() + "textures/ParallaxMapping/bricks_disp.jpg").c_str(), MappingTexParam);

    // tell OpenGl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    cubeShader.use(); // don't forget to activate/use the shader before setting uniforms!
    // either set it manually like so:
    glUniform1i(glGetUniformLocation(cubeShader.ID, "texture1"), 0);
    // or set via the texture class
    cubeShader.setInt("texture2", 1);

    cubeShader.setInt("material.diffuse", 0);
    cubeShader.setInt("material.specular", 1);
    cubeShader.setInt("material.emissive", 2);
    cubeShader.setInt("material.shadowMap", 3);
    cubeShader.setInt("material.shadowCubemap", 4);

    vegetationShader.use();
    vegetationShader.setInt("translucency", 0);

    screenShader.use();
    screenShader.setInt("screenTexture", 0);

    skyboxShader.setInt("skybox", textureCubemap);
    reflectionShader.setInt("skybox", 0);

    normalMappingShaderToWorld.use();
    normalMappingShaderToWorld.setInt("material.diffuseMap", 0);
    normalMappingShaderToWorld.setInt("material.normalMap", 1);

    normalMappingShaderToLocal.use();
    normalMappingShaderToLocal.setInt("material.diffuseMap", 0);
    normalMappingShaderToLocal.setInt("material.normalMap", 1);

    parallaxMappingShader.use();
    parallaxMappingShader.setInt("material.diffuseMap", 0);
    parallaxMappingShader.setInt("material.normalMap", 1);
    parallaxMappingShader.setInt("material.depthMap", 2);

    hdrScreenShader.use();
    hdrScreenShader.setInt("colorMap", 0);
    hdrScreenShader.setInt("bloomMap", 1);

    gaussianBlurShader.use();
    gaussianBlurShader.setInt("image", 0);

    ssaoShader.use();
    ssaoShader.setInt("gPosition", 0);
    ssaoShader.setInt("gNormal", 1);
    ssaoShader.setInt("texNoise", 2);

    ssaoBlurShader.use();
    ssaoBlurShader.setInt("ssaoInput", 0);

    lightingPassShader.use();
    lightingPassShader.setInt("gPosition", 0);
    lightingPassShader.setInt("gNormal", 1);
    lightingPassShader.setInt("gAlbedoSpec", 2);
    lightingPassShader.setInt("ssao", 3);

    pbrShader.use();
    pbrShader.setInt("irradianceMap", 0);
    pbrShader.setInt("prefilterMap", 1);
    pbrShader.setInt("brdfLUT", 2);
    pbrShader.setInt("albedoMap", 3);
    pbrShader.setInt("normalMap", 4);
    pbrShader.setInt("metallicMap", 5);
    pbrShader.setInt("roughnessMap", 6);
    pbrShader.setInt("aoMap", 7);

    glDisable(GL_BLEND);
    // IBL Diffuse preinitialize
    // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
    // ----------------------------------------------------------------------------------------------
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    equirectangularToCubemapShader.use();
    equirectangularToCubemapShader.setInt("equirectangularMap", 0);
    equirectangularToCubemapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, EquirectangularMap);

    // viewport settings
    glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        equirectangularToCubemapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance mode
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindFramebuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    // pbr: solve diffuse integral by convolution to create an irradiance (cube) map
    irradianceShader.use();
    irradianceShader.setInt("environmentMap", 0);
    irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // IBL: specular initialization
    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        unsigned int mipWidth = 128 * std::pow(0.5f, mip);
        unsigned int mipHeight = 128 * std::pow(0.5f, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glBindVertexArray(skyboxVAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    // pre - allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    brdfShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_BLEND);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Setup Dear ImGui Context
    ImGuiGLRenderer rendererUI;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window_))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        input.deltaTime = deltaTime;

        // input
        // -----
        input.processInput(window_);

        // render
        // ------

        // camera/view transformation
        glm::mat4 view;
        view = camera.GetViewMatrix();

        glm::mat4 projection;
        projection = glm::perspective(glm::radians(camera.Zoom), 800.0f / 600.0f, 0.1f, 100.0f);

        // update the uniform color
        float timeValue = glfwGetTime();
        float offsetValue = sin(timeValue) / 2.0f + 0.5f;

        lightPos.x = 1.0 + sin(glfwGetTime()) * 2.0f;
        lightPos.y = sin(glfwGetTime() / 2.0f) * 1.0f;

        //cubeShader.setMat4("view", view);
        //cubeShader.setMat4("projection", projection);
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));
        glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glDisable(GL_BLEND);
        // 1. geometry pass: render scene's geometry/color data into buffer
        // --------------------------------------------------------------
        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 worldModel = glm::mat4(1.0f);
        gBufferGeometryShader.use();
        for (unsigned int i = 0; i < objectPositions.size(); i++)
        {
            worldModel = glm::mat4(1.0f);
            worldModel = glm::translate(worldModel, objectPositions[i]);
            worldModel = glm::scale(worldModel, glm::vec3(0.5f));
            gBufferGeometryShader.setMat4("model", worldModel);
            backpackModel.Draw(gBufferGeometryShader);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // using GBuffer for creating SSAO texture
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        ssaoShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, noiseTexture);
        for (unsigned int i = 0; i < 64; i++)
            ssaoShader.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);
        ssaoShader.setMat4("projection", projection);
        glBindVertexArray(fullScreenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // SSAO Blur
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        ssaoBlurShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
        glBindVertexArray(fullScreenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. lightning pass: calculate lightning by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        lightingPassShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
        // send light relevant uniforms
        for (unsigned int i = 0; i < lightPositions.size(); i++)
        {
            glm::vec3 LightPosView = glm::vec3(view * glm::vec4(lightPositions[i], 1.0f));
            lightingPassShader.setVec3("lights[" + std::to_string(i) + "].Position", LightPosView);
            lightingPassShader.setVec3("lights[" + std::to_string(i) + "].Color", lightColors[i]);
            // Falloff
            glm::vec3 lightColor = lightColors[i];
            float constant = 1.0f;
            float linear = 0.7f;
            float quadratic = 1.8f;
            float lightMax = std::fmax(std::fmax(lightColor.r, lightColor.g), lightColor.b);
            float radius =
                (-linear + std::sqrtf(linear * linear - 4 * quadratic * (constant - 256.0f / 5.0f) * lightMax))
                / (2.0f * quadratic);
            lightingPassShader.setFloat("lights[" + std::to_string(i) + "].Radius", radius);
            lightingPassShader.setFloat("lights[" + std::to_string(i) + "].Linear", linear);
            lightingPassShader.setFloat("lights[" + std::to_string(i) + "].Quadratic", quadratic);
        }
        lightingPassShader.setVec3("viewPos", camera.Position);
        glBindVertexArray(fullScreenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // draw light cubes for forward rendering
        lightSourceShader.use();
        glm::mat4 lightModel;
        for (unsigned int i = 0; i < NR_LIGHTS; i++)
        {
            lightModel = glm::mat4();
            lightModel = glm::translate(lightModel, lightPositions[i]);
            lightModel = glm::scale(lightModel, glm::vec3(0.25f));
            lightSourceShader.setMat4("model", lightModel);
            lightSourceShader.setVec3("LightColor", lightColors[i]);
            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }

        glEnable(GL_BLEND);

        // Render depthMap pass
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        //glm::mat4 lightProjection = glm::perspective(glm::radians(90.0f), 1024.0f / 1024.0f, near_plane, far_plane);
        glm::mat4 lightView = glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        std::vector<MeshStructure> depthMapObjects;
        MeshStructure depthMapObject = MeshStructure();

        //depthMapObject.EBO = EBO;
        depthMapObject.VAO = VAO;
        //depthMapObject.shader = &depthMapShader;
        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, positions[i]);
            float angle = 20.0f * i;
            int y = i % 3;
            if (y == 0 || i == 0)
                angle = i == 0 ? 20 * (float)glfwGetTime() : angle * (float)glfwGetTime();
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            depthMapObject.meshesTransforms.push_back(model);
        }
        glm::mat4 modelFloor = glm::mat4(1.0f);
        modelFloor = glm::translate(modelFloor, glm::vec3(0.0f, 0.0f, -5.5f));
        modelFloor = glm::scale(modelFloor, glm::vec3(20.0f, 1.0f, 20.0f));
        depthMapObject.meshesTransforms.push_back(modelFloor);

        depthMapObjects.push_back(depthMapObject);
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        //glEnable(GL_CULL_FACE);
        //glCullFace(GL_FRONT);
        // render cubemap depth pass
        float aspect = (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT;
        float near = 1.0f;
        float far = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);

        glm::vec3 pointLightPos = choosePointLightPosition(static_cast<pointEnum>(3));
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj *
            glm::lookAt(pointLightPos, pointLightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj *
            glm::lookAt(pointLightPos, pointLightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj *
            glm::lookAt(pointLightPos, pointLightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj *
            glm::lookAt(pointLightPos, pointLightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj *
            glm::lookAt(pointLightPos, pointLightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj *
            glm::lookAt(pointLightPos, pointLightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));


        //RenderDepthMapScene(depthMapFBO, depthMapObjects, depthMapShader, lightSpaceMatrix, false, shadowTransforms);
        //glCullFace(GL_BACK);
        //glDisable(GL_CULL_FACE);


        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        RenderDepthMapScene(depthCubemapFBO, depthMapObjects, depthCubemapShader, lightSpaceMatrix, pointLightPos, false, shadowTransforms);

        // Render First Color Pass
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        // bind textures on corresponding units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, emissionMap);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        cubeShader.use();
        cubeShader.setFloat("offset", offsetValue);
        cubeShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        cubeShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
        cubeShader.setVec3("lightPos", pointLightPos);
        cubeShader.setVec3("viewPos", camera.Position);
        cubeShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        // shadow cubemap
        cubeShader.setFloat("far_plane", far_plane);

        glm::vec3 lightColor;
        lightColor.x = sin(glfwGetTime() * 2.0f);
        lightColor.y = sin(glfwGetTime() * 0.7f);
        lightColor.z = sin(glfwGetTime() * 1.3f);

        glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
        glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);

        // material struct
        cubeShader.setFloat("material.shininess", 32.0f);

        // light struct
        cubeShader.setVec3("light.position", camera.Position);
        cubeShader.setVec3("light.direction", camera.Front);
        cubeShader.setFloat("light.cutOff", glm::cos(glm::radians(12.5f)));
        cubeShader.setFloat("light.outerCutOff", glm::cos(glm::radians(17.5f)));
        cubeShader.setVec3("light.ambient", glm::vec3(0.2f));
        cubeShader.setVec3("light.diffuse", glm::vec3(0.5f));
        cubeShader.setVec3("light.specular", glm::vec3(1.0f));

        cubeShader.setFloat("light.constant", 1.0f);
        cubeShader.setFloat("light.linear", 0.09f);
        cubeShader.setFloat("light.quadratic", 0.032f);

        cubeShader.setVec3("dirLight.direction", glm::vec3(-0.5f, -0.5f, -0.5f));
        cubeShader.setVec3("dirLight.ambient", glm::vec3(0.2f));
        cubeShader.setVec3("dirLight.diffuse", glm::vec3(0.5f));
        cubeShader.setVec3("dirLight.specular", glm::vec3(1.0f));

        for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            std::string number = std::to_string(i);
            cubeShader.setVec3(("pointLights[" + number + "].position").c_str(), choosePointLightPosition((pointEnum)i));
            cubeShader.setFloat(("pointLights[" + number + "].constant").c_str(), 1.0f);
            cubeShader.setFloat(("pointLights[" + number + "].linear").c_str(), 0.09f / 4.0f);
            cubeShader.setFloat(("pointLights[" + number + "].quadratic").c_str(), 0.032f / 4.0f);
            cubeShader.setVec3(("pointLights[" + number + "].ambient").c_str(), glm::vec3(0.2f));
            cubeShader.setVec3(("pointLights[" + number + "].diffuse").c_str(), glm::vec3(0.5f));
            cubeShader.setVec3(("pointLights[" + number + "].specular").c_str(), glm::vec3(1.0f));
        }

        // use blinn-phong lighting model
        cubeShader.setBool("bUseBlinnPhong", bUseBlinnPhong);

        // create transformations
        glm::mat4 trans = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        trans = glm::translate(trans, glm::vec3(0.5f, -0.5f, 0.0f));
        trans = glm::rotate(trans, (float)glfwGetTime(), glm::vec3(0.0, 1.0, 0.0));

        // get matrix's uniform location and set matrix
        unsigned int transformLoc = glGetUniformLocation(cubeShader.ID, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));

        // render boxes
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, positions[i]);
            float angle = 20.0f * i;
            int y = i % 3;
            if (y == 0 || i == 0)
                angle = i == 0 ? 20 * (float)glfwGetTime() : angle * (float)glfwGetTime();
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            cubeShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glm::mat4 modelColorFloor = glm::mat4(1.0f);
        modelColorFloor = glm::translate(modelColorFloor, glm::vec3(0.0f, 0.0f, -5.5f));
        modelColorFloor = glm::scale(modelColorFloor, glm::vec3(20.0f, 1.0f, 20.0f));
        cubeShader.setMat4("model", modelColorFloor);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // PBR
        glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        pbrShader.use();
        // bind pre-computed IBL data
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, PBR_AlbedoMap);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, PBR_NormalMap);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, PBR_MetallicMap);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, PBR_RoughnessMap);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, PBR_AOMap);

        pbrShader.setVec3("CamPos", camera.Position);

        // render rows*column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
        glm::mat4 pbrModel = glm::mat4(1.0f);
        pbrModel = glm::translate(pbrModel, glm::vec3(0.0f, 5.0f, 0.0f));
        pbrModel = glm::rotate(pbrModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        pbrModel = glm::rotate(pbrModel, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        pbrModel = glm::scale(pbrModel, glm::vec3(0.2f));
        pbrShader.setMat4("model", pbrModel);
        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(pbrModel))));
        gunModel.Draw(pbrShader);
        //for (int row = 0; row < nrRows; ++row)
        //{
        //    pbrShader.setFloat("metallic", float(row) / (float)nrRows);
        //    for (int col = 0; col < nrColumns; ++col)
        //    {
        //        // we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
        //        // on direct lighting.
        //        pbrShader.setFloat("roughness", std::clamp(float(col) / float(nrColumns), 0.05f, 1.0f));

        //        pbrModel = glm::mat4(1.0f);
        //        pbrModel = glm::translate(pbrModel, glm::vec3(
        //            (col - (nrColumns / 2)) * spacing,
        //            (row - (nrRows / 2)) * spacing,
        //            0.0f
        //        ));
        //        pbrShader.setMat4("model", pbrModel);
        //        pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(pbrModel))));
        //        renderSphere();
        //    }
        //}

        // render light source (simply re-render sphere at light positions)
        // this looks a bit off as we use the same shader, but it'll make their positions obvious and 
        // keeps the codeprint small.
        for (unsigned int i = 0; i < sizeof(pbrLightPositions) / sizeof(pbrLightPositions[0]); ++i)
        {
            glm::vec3 newPos = pbrLightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0f) * 5.0f, 0.0f, 0.0f);
            newPos = pbrLightPositions[i];
            pbrShader.setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
            pbrShader.setVec3("lightColors[" + std::to_string(i) + "]", pbrLightColors[i]);

            pbrModel = glm::mat4(1.0f);
            pbrModel = glm::translate(pbrModel, newPos);
            pbrModel = glm::scale(pbrModel, glm::vec3(0.5f));
            pbrShader.setMat4("model", pbrModel);
            pbrShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(pbrModel))));
            renderSphere();
        }

        // render skybox (render as last to prevent overdraw)
        /*glDepthFunc(GL_LEQUAL);
        backgroundIBLShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS)*/;

        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, normalMappingDiffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalMappingNormalMap);
        normalMappingShaderToWorld.use();
        glm::mat4 normalMappingModelToWorld = glm::mat4(1.0f);
        normalMappingModelToWorld = glm::translate(normalMappingModelToWorld, glm::vec3(0.0f, 3.0f, -2.0f));
        normalMappingModelToWorld = glm::rotate(normalMappingModelToWorld, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        normalMappingShaderToWorld.setMat4("model", normalMappingModelToWorld);
        normalMappingShaderToWorld.setVec3("viewPos", camera.Position);
        normalMappingShaderToWorld.setFloat("material.shininess", 32.0f);
        normalMappingShaderToWorld.setBool("bUseBlinnPhong", bUseBlinnPhong);
        normalMappingShaderToWorld.setVec3("pointLight.position", choosePointLightPosition((pointEnum)2));
        normalMappingShaderToWorld.setFloat("pointLight.constant", 1.0f);
        normalMappingShaderToWorld.setFloat("pointLight.linear", 0.09f);
        normalMappingShaderToWorld.setFloat("pointLight.quadratic", 0.032f);
        normalMappingShaderToWorld.setVec3("pointLight.ambient", glm::vec3(0.2f));
        normalMappingShaderToWorld.setVec3("pointLight.diffuse", glm::vec3(0.5f));
        normalMappingShaderToWorld.setVec3("pointLight.specular", glm::vec3(1.0f));
        renderTBNQuad();

        normalMappingShaderToLocal.use();
        glm::mat4 normalMappingModelToLocal = glm::mat4(1.0f);
        normalMappingModelToLocal = glm::translate(normalMappingModelToLocal, glm::vec3(1.0f, 2.0f, 0.0f));
        normalMappingModelToLocal = glm::rotate(normalMappingModelToLocal, glm::radians(45.0f), glm::vec3(1.0f, .0f, 0.0f));
        normalMappingModelToLocal = glm::rotate(normalMappingModelToLocal, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        normalMappingShaderToLocal.setMat4("model", normalMappingModelToLocal);
        normalMappingShaderToLocal.setVec3("viewPos", camera.Position);
        normalMappingShaderToLocal.setFloat("material.shininess", 32.0f);
        normalMappingShaderToLocal.setBool("bUseBlinnPhong", bUseBlinnPhong);
        normalMappingShaderToLocal.setVec3("pointLight.position", choosePointLightPosition((pointEnum)3));
        normalMappingShaderToLocal.setFloat("pointLight.constant", 1.0f);
        normalMappingShaderToLocal.setFloat("pointLight.linear", 0.09f);
        normalMappingShaderToLocal.setFloat("pointLight.quadratic", 0.032f);
        normalMappingShaderToLocal.setVec3("pointLight.ambient", glm::vec3(0.2f));
        normalMappingShaderToLocal.setVec3("pointLight.diffuse", glm::vec3(0.5f));
        normalMappingShaderToLocal.setVec3("pointLight.specular", glm::vec3(1.0f));
        renderTBNQuad();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, parallaxMappingDiffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, parallaxMappingNormalMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, parallaxMappingDepthMap);
        parallaxMappingShader.use();
        glm::mat4 parallaxMappingModel = glm::mat4(1.0f);
        parallaxMappingModel = glm::translate(parallaxMappingModel, glm::vec3(0.0f, 3.0f, 0.0f));
        parallaxMappingModel = glm::rotate(parallaxMappingModel, glm::radians(45.0f), glm::vec3(1.0f, .0f, 0.0f));
        parallaxMappingModel = glm::rotate(parallaxMappingModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        parallaxMappingShader.setMat4("model", parallaxMappingModel);
        parallaxMappingShader.setVec3("viewPos", camera.Position);
        parallaxMappingShader.setFloat("material.shininess", 32.0f);
        parallaxMappingShader.setBool("bUseBlinnPhong", bUseBlinnPhong);
        parallaxMappingShader.setVec3("pointLight.position", choosePointLightPosition((pointEnum)3));
        parallaxMappingShader.setFloat("pointLight.constant", 1.0f);
        parallaxMappingShader.setFloat("pointLight.linear", 0.09f);
        parallaxMappingShader.setFloat("pointLight.quadratic", 0.032f);
        parallaxMappingShader.setVec3("pointLight.ambient", glm::vec3(0.2f));
        parallaxMappingShader.setVec3("pointLight.diffuse", glm::vec3(0.5f));
        parallaxMappingShader.setVec3("pointLight.specular", glm::vec3(1.0f));
        renderTBNQuad();



        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDisable(GL_DEPTH_TEST);
        stencilShader.use();

        stencilShader.setMat4("projection", projection);
        stencilShader.setMat4("view", view);
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));

        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, positions[i]);
            float angle = 20.0f * i;
            int y = i % 3;
            if (y == 0 || i == 0)
                angle = i == 0 ? 20 * (float)glfwGetTime() : angle * (float)glfwGetTime();
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            model = glm::scale(model, glm::vec3(1.05f, 1.05f, 1.05f));
            stencilShader.setMat4("model", model);

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
        glEnable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, lightPos);
        model = glm::scale(model, glm::vec3(0.2f));
        lightingSourceShader.use();
        lightingSourceShader.setMat4("model", model);
        lightingSourceShader.setMat4("projection", projection);
        lightingSourceShader.setMat4("view", view);

        // material struct
        lightingSourceShader.setVec3("material.ambient", ambientColor.x * 1.0f, ambientColor.y * 1.0f, ambientColor.y * 1000.0f);

        // light struct
        lightingSourceShader.setVec3("light.ambient", 1.0f, 1.0f, 1.0f);

        // draw the light cube object
        glBindVertexArray(lightVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightEBO);
        glDrawElements(GL_POINTS, 36, GL_UNSIGNED_INT, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // draw the light cube object
        glBindVertexArray(lightVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightEBO);
        glDrawElements(GL_POINTS, 36, GL_UNSIGNED_INT, 0);

        bool horizontal = true, first_iteration = true;
        int amountPingpong = 10;
        gaussianBlurShader.use();
        for(unsigned int i = 0; i < amountPingpong; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            gaussianBlurShader.setInt("horizontal", horizontal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorTextures[1] : pingpongBuffer[!horizontal]);

            glBindVertexArray(fullScreenVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, translucencyMap);

        vegetationShader.use();
        vegetationShader.setMat4("projection", projection);
        vegetationShader.setMat4("view", view);

        glBindVertexArray(vegetationsVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vegetationsEBO);

        std::map<float, glm::vec3> sorted;
        for(unsigned int i = 0; i < positionVegetations.size(); i++)
        {
            float distance = glm::length(camera.Position - positionVegetations[i]);
            sorted[distance] = positionVegetations[i];
        }

        for(std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            vegetationShader.setMat4("model", model);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CCW);

        cullingShader.use();
        cullingShader.setMat4("projection", projection);
        cullingShader.setMat4("view", view);
        glm::mat4 cullingModel = glm::mat4(1.0f);
        cullingModel = glm::translate(cullingModel, glm::vec3(0.0f, -3.5f, 0.0f));
        cullingShader.setMat4("model", cullingModel);

        glBindVertexArray(cullingVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDisable(GL_CULL_FACE);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        modelShader.use();
        modelShader.setMat4("view", view);
        modelShader.setMat4("projection", projection);
        glm::mat4 modelBackpack = glm::mat4(1.0f);
        modelBackpack = glm::translate(modelBackpack, glm::vec3(0.f, 0.f, 0.f));
        modelBackpack = glm::scale(modelBackpack, glm::vec3(1.f, 1.f, 1.f));
        modelShader.setMat4("model", modelBackpack);
        modelShader.setVec3("viewPos", camera.Position);
        modelShader.setFloat("material.shininess", 32.0f);
        modelShader.setBool("bUseBlinnPhong", bUseBlinnPhong);
        modelShader.setVec3("pointLight.position", glm::vec3(0.0f, 2.0f, 2.0f));
        modelShader.setFloat("pointLight.constant", 1.0f);
        modelShader.setFloat("pointLight.linear", 0.09f);
        modelShader.setFloat("pointLight.quadratic", 0.032f);
        modelShader.setVec3("pointLight.ambient", glm::vec3(0.2f));
        modelShader.setVec3("pointLight.diffuse", glm::vec3(0.5f));
        modelShader.setVec3("pointLight.specular", glm::vec3(1.0f));
        backpackModel.Draw(modelShader);

        // draw planet
        simpleModelShader.use();
        simpleModelShader.setMat4("view", view);
        simpleModelShader.setMat4("projection", projection);
        glm::mat4 modelPlanet = glm::mat4(1.0f);
        modelPlanet = glm::translate(modelPlanet, glm::vec3(0.0f, -10.0f, 0.0f));
        modelPlanet = glm::scale(modelPlanet, glm::vec3(1.0f, 1.0f, 1.0f));
        simpleModelShader.setMat4("model", modelPlanet);
        planetModel.Draw(simpleModelShader);

        modelInstanceShader.use();
        modelInstanceShader.setMat4("view", view);
        modelInstanceShader.setMat4("projection", projection);
        modelInstanceShader.setInt("texture_diffuse1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rockModel.GetTexturesLoaded()[0].id);
        // draw meteorites
        for (unsigned int i = 0; i < rockMeshes.size(); i++)
        {
            glBindVertexArray(rockMeshes[i]->VAO);
            glDrawElementsInstanced(GL_TRIANGLES, static_cast<unsigned int>(rockMeshes[i]->indices.size()), GL_UNSIGNED_INT, 0, amount);
            glBindVertexArray(0);
        }

        /*modelShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            modelShader.setMat4("model", modelMatrices[i]);
            rockModel.Draw(modelShader);
        }*/

#if USE_NORMALS == 1
        normalModelShader.use();
        normalModelShader.setMat4("view", view);
        normalModelShader.setMat4("projection", projection);
        normalModelShader.setMat4("model", modelBackpack);
        backpackModel.Draw(normalModelShader);
#endif

        simpleInstanceShader.use();

        glBindVertexArray(quadInstancedVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 100);


        reflectionShader.use();
        reflectionShader.setMat4("projection", projection);
        reflectionShader.setMat4("view", view);
        glm::mat4 reflectionModel = glm::mat4(1.0f);
        reflectionModel = glm::translate(reflectionModel, glm::vec3(3.0f));
        reflectionShader.setMat4("model", reflectionModel);
        reflectionShader.setVec3("cameraPos", camera.Position);
        glBindVertexArray(reflectionVAO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureCubemap);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        refractionShader.use();
        refractionShader.setMat4("projection", projection);
        refractionShader.setMat4("view", view);
        glm::mat4 refractionModel = glm::mat4(1.0f);
        refractionModel = glm::translate(refractionModel, glm::vec3(-3.0f));
        refractionShader.setMat4("model", refractionModel);
        refractionShader.setVec3("cameraPos", camera.Position);
        glBindVertexArray(refractionVAO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureCubemap);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        //glDepthMask(GL_FALSE);

        // Skybox
        // ----------------------------------
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        // camera/view transformation
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxEBO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureCubemap);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        GeometryHouseShader.use();
        glBindVertexArray(GeometryHousesVAO);
        glDrawArrays(GL_POINTS, 0, 4);
        //glDepthMask(GL_FALSE);

        // camera/view transformation
        camera.Yaw += 180.0f;
        camera.ProcessMouseMovement(0, 0, false);

        std::vector<MeshStructure> defaultLitObjects;
        MeshStructure defaultLitObject = MeshStructure();

        DefaultLitModelParams defaultLitModelParams = DefaultLitModelParams();
        defaultLitObject.defaultLitShaderParams = &defaultLitModelParams;

        FMaterial material = FMaterial(diffuseMap, specularMap, emissionMap, -1, 32.0f);
        defaultLitObject.material = &material;

        FDirectionalLight dirLight = FDirectionalLight(glm::vec3(-0.5f), glm::vec3(0.2f), glm::vec3(0.5f), glm::vec3(1.0f));
        defaultLitObject.defaultLitShaderParams->dirLight = &dirLight;

        //defaultLitObject.EBO = EBO;
        defaultLitObject.VAO = VAO;
        defaultLitObject.shader = &cubeShader;

        for (unsigned int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            defaultLitObject.defaultLitShaderParams->pointLights.push_back(
                new FPointLightStructure(choosePointLightPosition(static_cast<pointEnum>(i)),
                    glm::vec3(0.2f), glm::vec3(0.5f), glm::vec3(1.0f),
                    1.0f, 0.09f, 0.032f));
        }

        for (unsigned int i = 0; i < 10; i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, positions[i]);
            float angle = 20.0f * i;
            int y = i % 3;
            if (y == 0 || i == 0)
                angle = i == 0 ? 20 * (float)glfwGetTime() : angle * (float)glfwGetTime();
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            defaultLitObject.meshesTransforms.push_back(model);
        }
        defaultLitObjects.push_back(defaultLitObject);

        RenderScene(msFBO, &camera, defaultLitObjects, std::vector<MeshStructure>(), std::vector<MeshStructure>(), false, false);

        camera.Yaw -= 180.0f;
        camera.ProcessMouseMovement(0, 0, false);

        // resolving multisample buffer with additional buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

#if USE_POST_PROCESSING == 1
        // render
    // ------
    // bind to framebuffer and draw scene as we normally would to color texture 

    // make sure we clear the framebuffer's content
        glBindVertexArray(0);

        // now bind back to default framebuffer and draw a quad plane with the attached framebuffer color texture
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
    // clear all relevant buffers
        //glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
        //glClear(GL_COLOR_BUFFER_BIT);

        screenShader.use();
        glBindVertexArray(screenVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
        glDrawArrays(GL_TRIANGLES, 0, 6);
#elif USE_POST_PROCESSING == 2
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        depthMapDebugScreenShader.use();
        depthMapDebugScreenShader.setFloat("near_plane", 1.0f);
        depthMapDebugScreenShader.setFloat("far_plane", 12.5f);
        glBindVertexArray(screenVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glDrawArrays(GL_TRIANGLES, 0, 6);
#elif USE_POST_PROCESSING == 3
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(0);

        hdrScreenShader.use();
        glBindVertexArray(fullScreenVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTextures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[!horizontal]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
#endif

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //ImGui_ImplOpenGL3_NewFrame();
        //ImGui_ImplGlfw_NewFrame();
        int ImGuiWidth, ImGuiHeight;
        glfwGetFramebufferSize(window_, &ImGuiWidth, &ImGuiHeight);
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(ImGuiWidth, ImGuiHeight);

        ImGui::NewFrame();
        //ImGui::ShowDemoWindow();
        ImGui::Begin("start window");
        ImGui::SetWindowSize(ImVec2(500.0f, 200.0f));
        ImGui::Text("This is some useful text.");

        //ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", deltaTime, 1.0f / deltaTime);
        ImGui::End();
        ImGui::EndFrame();

        ImGui::Render();
        rendererUI.render(ImGuiWidth, ImGuiHeight, ImGui::GetDrawData());

        glScissor(0, 0, ImGuiWidth, ImGuiHeight);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);


        const char* const sl_TaskflowProcessing = "Taskflow Processing";

        FrameMarkStart(sl_TaskflowProcessing);

        tf::Taskflow taskflow;

        std::vector<int> items{ 1, 2, 3, 4, 5, 6, 7, 8 };

        auto task = taskflow.for_each(
            items.begin(), items.end(), [&](int item)
            {
                std::cout << item;
            }
        ).name("R");
        
        taskflow.emplace([]() { std::cout << "\nS - Start\n"; }).name("S").precede(task);
        taskflow.emplace([]() { std::cout << "\nT - End\n"; }).name("T").succeed(task);

        {
            std::ofstream os("taskflow.dot");
            taskflow.dump(os);
        }

        tf::Executor executor;
        executor.run(taskflow).wait();

        FrameMarkEnd(sl_TaskflowProcessing);

        if(app_) app_->swapBuffers();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    return 0;
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path, const Texture2DParam& TexParam)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    // load image, create texture and generate the mipmaps
    stbi_set_flip_vertically_on_load(TexParam.bFlipVertical); // tell stb_image.h to flip loaded texture's on the y-axis.
    void* data = nullptr;
    if(TexParam.bLoadHDR)
    {
        data = stbi_loadf(path, &width, &height, &nrChannels, 0);
    }
    else
    {
        data = stbi_load(path, &width, &height, &nrChannels, 0);
    }
    if (data)
    {
        GLenum format = TexParam.Format;
        GLint internalFormat = TexParam.InternalFormat;
        if (TexParam.Format == GL_NONE)
        {
            if (nrChannels == 1)
                format = GL_RED;
            else if (nrChannels == 3)
                format = GL_RGB;
            else if (nrChannels == 4)
                format = GL_RGBA;
        }
        if (TexParam.InternalFormat == GL_NONE) internalFormat = static_cast<GLint>(format);

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0,
            internalFormat,
            width, height, 0, format, TexParam.TypeData, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TexParam.WrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TexParam.WrapT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TexParam.MinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TexParam.MagFilter);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false); // tell stb_image.h to flip loaded texture's on the y-axis.
    int width, height, nrChannels;
    unsigned char* data;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int sphereVAO = 0;
unsigned int indexCount = 0;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uvs.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }
        bool oddRow = false;
        for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uvs.size() > 0)
            {
                data.push_back(uvs[i].x);
                data.push_back(uvs[i].y);
            }
        }

        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}