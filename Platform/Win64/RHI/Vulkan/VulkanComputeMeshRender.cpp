#include <RHI/Vulkan/VulkanComputeMeshRender.hpp>

#include <UserInput/GLFW/GLFWUserInput.hpp>
#include <RHI/Vulkan/Framework/VulkanApp.hpp>
#include <RHI/Vulkan/Renderers/VulkanClear.hpp>
#include <RHI/Vulkan/Renderers/VulkanComputedVB.hpp>
#include <RHI/Vulkan/Renderers/VulkanComputedImage.hpp>
#include <RHI/Vulkan/Renderers/VulkanFinish.hpp>
#include <RHI/Vulkan/Renderers/VulkanModelRenderer.hpp>
#include <RHI/Vulkan/Renderers/VulkanImGui.hpp>
#include <Filesystem/FilesystemUtilities.hpp>

/// should contain at least two elements
std::deque<std::pair<uint32_t, uint32_t>> morphQueue = { {5, 8}, {5, 8} };

/// morphing between two torus knots 0..1
float morphCoef = 0.0f;
float animationSpeed = 1.0f;

bool useColoredMesh = false;

const uint32_t numU = 1024;
const uint32_t numV = 1024;

struct MeshUniformBuffer
{
    float time;
    uint32_t numU;
    uint32_t numV;
    float minU, maxU;
    float minV, maxV;
    uint32_t p1, p2;
    uint32_t q1, q2;
    float morph;
} meshUBO;

void generateIndices(uint32_t* indices)
{
    for(uint32_t j = 0; j < numV - 1; j++)
    {
        for(uint32_t i = 0; i < numU - 1; i++)
        {
            uint32_t ofs = (j * (numU - 1) + i) * 6;

            uint32_t i1 = (j + 0) * numU + (i + 0);
            uint32_t i2 = (j + 0) * numU + (i + 1);
            uint32_t i3 = (j + 1) * numU + (i + 1);
            uint32_t i4 = (j + 1) * numU + (i + 0);

            indices[ofs + 0] = i1;
            indices[ofs + 1] = i2;
            indices[ofs + 2] = i4;

            indices[ofs + 3] = i2;
            indices[ofs + 4] = i3;
            indices[ofs + 5] = i4;
        }
    }
}

void VulkanComputeMeshRender::initMesh()
{
    std::vector<uint32_t> indicesGen((numU - 1) * (numV - 1) * 6);

    generateIndices(indicesGen.data());

    uint32_t vertexBufferSize = 12 * sizeof(float) * numU * numV;
    uint32_t indexBufferSize = sizeof(uint32_t) * (numU - 1) * (numV - 1) * 6;
    uint32_t bufferSize = vertexBufferSize + indexBufferSize;

    imgGen = std::make_unique<ComputedImage>(vkDev, (FilesystemUtilities::GetShadersDir() + "Vulkan/VK04_compute_texture.comp").c_str(), 1024, 1024, false);
    meshGen = std::make_unique<ComputedVertexBuffer>(vkDev, (FilesystemUtilities::GetShadersDir() + "Vulkan/VK04_compute_mesh.comp").c_str(), indexBufferSize, sizeof(MeshUniformBuffer), 12 * sizeof(float), numU * numV);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(vkDev.device, vkDev.physicalDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    {
        void* data;
        vkMapMemory(vkDev.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        // pregenerated index data
        memcpy((void*)((uint8_t*)data + vertexBufferSize), indicesGen.data(), indexBufferSize);
        vkUnmapMemory(vkDev.device, stagingBufferMemory);
    }

    copyBuffer(vkDev, stagingBuffer, meshGen->computedBuffer, bufferSize);

    vkDestroyBuffer(vkDev.device, stagingBuffer, nullptr);
    vkFreeMemory(vkDev.device, stagingBufferMemory, nullptr);

    meshGen->fillComputeCommandBuffer();
    meshGen->submit();
    vkDeviceWaitIdle(vkDev.device);

    std::string vertShader = (FilesystemUtilities::GetShadersDir() + "Vulkan/VK04_render.vert");
    std::string fragShader = (FilesystemUtilities::GetShadersDir() + "Vulkan/VK04_render.frag");
    std::string fragColorShader = (FilesystemUtilities::GetShadersDir() + "Vulkan/VK04_render_color.frag");
    std::vector<const char*> shaders = { vertShader.c_str(), fragShader.c_str() };
    std::vector<const char*> shadersColor = { vertShader.c_str(), fragColorShader.c_str() };

    mesh = std::make_unique<ModelRenderer>(
            vkDev, true,
            meshGen->computedBuffer, meshGen->computedMemory,
            vertexBufferSize, indexBufferSize,
            imgGen->computed, imgGen->computedImageSampler,
            shaders, (uint32_t)sizeof(mat4),
            true);

    meshColor = std::make_unique<ModelRenderer>(
            vkDev, true,
            meshGen->computedBuffer, meshGen->computedMemory,
            vertexBufferSize, indexBufferSize,
            imgGen->computed, imgGen->computedImageSampler,
            shadersColor, (uint32_t)sizeof(mat4),
            true, mesh->getDepthTexture(), false);
}

bool VulkanComputeMeshRender::initVulkan()
{
    createInstance(&vk_.instance);

    BL_CHECK(setupDebugCallbacks(vk_.instance, &vk_.messenger, &vk_.reportCallback));

    VK_CHECK(glfwCreateWindowSurface(vk_.instance, window_, nullptr, &vk_.surface));

    BL_CHECK(initVulkanRenderDeviceWithCompute(vk_, vkDev, kScreenWidth, kScreenHeight, VkPhysicalDeviceFeatures{}));

    initMesh();

    clear = std::make_unique<VulkanClear>(vkDev, mesh->getDepthTexture());
    finish = std::make_unique<VulkanFinish>(vkDev, mesh->getDepthTexture());
    imgui = std::make_unique<ImGuiRenderer>(vkDev);

    return VK_SUCCESS;
}

void VulkanComputeMeshRender::terminateVulkan()
{
    destroyVulkanImage(vkDev.device, imgGen->computed);
    vkDestroySampler(vkDev.device, imgGen->computedImageSampler, nullptr);

    clear = nullptr;
    finish = nullptr;

    imgui = nullptr;
    meshGen = nullptr;
    imgGen = nullptr;
    mesh = nullptr;
    meshColor->freeTextureSampler(); // release sampler handle
    meshColor = nullptr;

    destroyVulkanRenderDevice(vkDev);
    destroyVulkanInstance(vk_);
}

void VulkanComputeMeshRender::recreateSwapchain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window_, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(vkDev.device);

    cleanupSwapchain();
    vkDev.deviceQueueIndices.clear();

    initVulkanRenderDeviceWithCompute(vk_, vkDev, width, height, VkPhysicalDeviceFeatures{});

    initMesh();

    clear = std::make_unique<VulkanClear>(vkDev, mesh->getDepthTexture());
    finish = std::make_unique<VulkanFinish>(vkDev, mesh->getDepthTexture());
    imgui = std::make_unique<ImGuiRenderer>(vkDev);
}

void VulkanComputeMeshRender::cleanupSwapchain()
{
    destroyVulkanImage(vkDev.device, imgGen->computed);
    vkDestroySampler(vkDev.device, imgGen->computedImageSampler, nullptr);

    clear = nullptr;
    finish = nullptr;

    imgui = nullptr;
    meshGen = nullptr;
    imgGen = nullptr;
    mesh = nullptr;
    meshColor->freeTextureSampler(); // release sampler handle
    meshColor = nullptr;

    destroyVulkanRenderDevice(vkDev);
}


void VulkanComputeMeshRender::renderGUI(uint32_t imageIndex)
{
#if defined(WIN32)
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
#elif defined(__APPLE__)
    int width = 1600, height = 900;
#endif

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    ImGui::NewFrame();

//    const ImGuiWindowFlags flags =
//        ImGuiWindowFlags_NoTitleBar |
//        ImGuiWindowFlags_NoResize |
//        ImGuiWindowFlags_NoMove |
//        ImGuiWindowFlags_NoScrollbar |
//        ImGuiWindowFlags_NoSavedSettings |
//        ImGuiWindowFlags_NoInputs;
//        ImGuiWindowFlags_NoBackground;

    // Each torus knot is specified by a pair of coprime integers p and q.
    // https://en.wikipedia.org/wiki/Torus_knot
    static const std::vector<std::pair<uint32_t, uint32_t>> PQ = {
            {2, 3}, {2, 5}, {2, 7}, {3, 4}, {2, 9}, {3, 5}, {5 ,8}
    };

    ImGui::Begin("Torus Knot params", nullptr);
    {
        ImGui::Checkbox("Use colored mesh", &useColoredMesh);
        ImGui::SliderFloat("Animation speed", &animationSpeed, 0.0f, 2.0f);

        for(size_t i = 0; i != PQ.size(); i++)
        {
            std::string title = std::to_string(PQ[i].first) + ", " + std::to_string(PQ[i].second);
            if(ImGui::Button(title.c_str()))
            {
                if (PQ[i] != morphQueue.back())
                    morphQueue.push_back(PQ[i]);
            }
        }
    }
    ImGui::End();
    ImGui::Render();

    imgui->updateBuffers(vkDev, imageIndex, ImGui::GetDrawData());
}

void VulkanComputeMeshRender::updateBuffers(uint32_t imageIndex)
{
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    const float ratio = width / (float)height;

    const mat4 m1 = glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, -18.0f));
    const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
    const mat4 mtx = p * m1;

    if (useColoredMesh)
        meshColor->updateUniformBuffer(vkDev, imageIndex, glm::value_ptr(mtx), sizeof(mat4));
    else
        mesh->updateUniformBuffer(vkDev, imageIndex, glm::value_ptr(mtx), sizeof(mat4));

    renderGUI(imageIndex);
}

void VulkanComputeMeshRender::composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    clear->fillCommandBuffer(commandBuffer, imageIndex);

    if (useColoredMesh)
        meshColor->fillCommandBuffer(commandBuffer, imageIndex);
    else
        mesh->fillCommandBuffer(commandBuffer, imageIndex);

    imgui->fillCommandBuffer(commandBuffer, imageIndex);
    finish->fillCommandBuffer(commandBuffer, imageIndex);
}

VulkanComputeMeshRender::VulkanComputeMeshRender(const uint32_t screenWidth, uint32_t screenHeight)
        : VulkanBaseRender(screenWidth, screenHeight)
{
    glslang_initialize_process();

    volkInitialize();

    if (!glfwInit())
        exit(EXIT_FAILURE);

    if (!glfwVulkanSupported())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    window_ = glfwCreateWindow(kScreenWidth, kScreenHeight, "VulkanApp", nullptr, nullptr);
    if (!window_)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    input_ = &GLFWUserInput::GetInput();
}

float easing(float x)
{
    return (x < 0.5)
           ? (4 * x * x * (3 * x - 1))
           : (4 * (x - 1) * (x - 1) * (3 * (x - 1) + 1) + 1);
}

int VulkanComputeMeshRender::draw()
{
    initVulkan();

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window_))
    {
        auto iter = morphQueue.begin();

        meshUBO.time = (float)glfwGetTime();
        meshUBO.morph = easing(morphCoef);
        meshUBO.p1 = iter->first;
        meshUBO.q1 = iter->second;
        meshUBO.p2 = (iter + 1)->first;
        meshUBO.q2 = (iter + 1)->second;

        meshUBO.numU = numU;
        meshUBO.numV = numV;
        meshUBO.minU = -1.0f;
        meshUBO.maxU = 1.0f;
        meshUBO.minV = -1.0f;
        meshUBO.maxV = 1.0f;

        meshGen->uploadUniformBuffer(sizeof(MeshUniformBuffer), &meshUBO);

        meshGen->fillComputeCommandBuffer(nullptr, 0, meshGen->computedVertexCount / 2, 1, 1);
        meshGen->submit();
        vkDeviceWaitIdle(vkDev.device);

        imgGen->fillComputeCommandBuffer(&meshUBO.time, sizeof(float), imgGen->computedWidth / 16, imgGen->computedHeight / 16, 1);
        imgGen->submit();
        vkDeviceWaitIdle(vkDev.device);

        auto drawResult = drawFrame(vkDev,
                                    [=](uint32_t imageIndex) { updateBuffers(imageIndex); },
                                    [=](VkCommandBuffer commandBuffer, uint32_t imageIndex) { this->composeFrame(commandBuffer, imageIndex); });

        if(!drawResult)
        {
            recreateSwapchain();
        }

        const double newTime = glfwGetTime();
        const float deltaSeconds = static_cast<float>(newTime - lastTime);
        lastTime = newTime;
        morphCoef += animationSpeed * deltaSeconds;
        if(morphCoef > 1.0)
        {
            morphCoef = 1.0f;
            if(morphQueue.size() > 2)
            {
                morphCoef = 0.0f;
                morphQueue.pop_front();
            }
        }

        glfwPollEvents();
    }

    terminateVulkan();

    ImGui::DestroyContext();

    glfwTerminate();

    glslang_finalize_process();

    return 0;
}