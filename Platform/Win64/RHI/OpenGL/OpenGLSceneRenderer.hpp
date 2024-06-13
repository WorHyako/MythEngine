#pragma once
#include <RHI/OpenGL/OpenGLBaseRender.hpp>
#include <OpenGLUtils.hpp>
#include <Camera/BaseCamera.hpp>

class OpenGLSceneRenderer : public OpenGLBaseRender
{
public:
    OpenGLSceneRenderer(GLApp* app);
    ~OpenGLSceneRenderer() override = default;

    virtual int draw() override;

private:
    void RenderScene(
        const unsigned int framebuffer,
        Camera* const camera,
        const std::vector<MeshStructure>& defaultLitObjects,
        const std::vector<MeshStructure>& unlitObjects,
        const std::vector<MeshStructure>& translucentObjects,
        const bool bDrawStencil,
        const bool bUseCulling);
    void RenderDepthMapScene(
        const unsigned int framebuffer,
        std::vector<MeshStructure>& depthMapObjects,
        Shader& depthMapShader,
        const glm::mat4& lightSpaceMatrix,
        const glm::vec3& lightPos,
        const bool bUseCulling,
        const std::vector<glm::mat4> shadowTransforms);
    void DrawOutline(MeshStructure& renderObject, glm::mat4 view, glm::mat4 projection);
    void renderSphere();
    void renderTBNQuad();
    void renderQuad();

private:
    bool bUseBlinnPhong = false;

    // timing
    float deltaTime = 0.0f; // time between current frame and last frame
    float lastFrame = 0.0f; // time at last frame

    // camera
    //Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
    glm::vec3 cameraPos = glm::vec3{ 0.0f, 0.0f, 3.0f };
    glm::vec3 cameraFront = glm::vec3{ 0.0f, 0.0f, -1.0f };
    glm::vec3 cameraUp = glm::vec3{0.0f, 1.0f, 0.0f };

    // light
    glm::vec3 lightPos = glm::vec3{ 1.2f, 1.0f, 2.0f };

    unsigned int sphereVAO = 0;
    unsigned int indexCount = 0;
    unsigned int TBNQuadVAO = 0;
    unsigned int TBNQuadVBO = 0;
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;
};