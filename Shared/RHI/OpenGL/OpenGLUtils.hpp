#pragma once

#include <OpenGLBaseShader.hpp>
#include <OpenGLBaseModel.hpp>

#include <string>
#include <vector>

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

    Texture2DParameter();
    void SetWrap(int WrapMode)
    {
        WrapS = WrapMode;
        WrapT = WrapMode;
    }
} typedef Texture2DParam;

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

enum
{
    OnlyPos = 3,
    PosNormal = 6,
    PosNormalUV = 8,
    PosUV = 5
}typedef typeOfVertex;

namespace OpenGLUtils
{
    unsigned int loadTexture2D(char const* path, const Texture2DParam& TexParam);
    unsigned int loadCubemap(std::vector<std::string> faces);

    void createTextureAttachment(unsigned int& texture, const unsigned int width, const unsigned int height);
    void createTextureMultisampleAttachment(unsigned int& texture, const float sample, const unsigned int width, const unsigned int height);
    void createRenderBufferObject(unsigned int& rbo, const unsigned int width, const unsigned int height);
    void createRenderBufferMultisampleObject(unsigned int& rbo, const unsigned int width, const unsigned int height);
    void createVAO(
        unsigned int& VAO,
        unsigned int& VBO,
        unsigned int& EBO,
        typeOfVertex typeVertex,
        std::vector<float> vertices,
        unsigned int sizeOfVertex,
        std::vector<unsigned int> indices = std::vector<unsigned int>());
    void bindBuffer(unsigned int& buffer);
}