#include <RHI/Vulkan/Vulkan_GLTF_PBRRender.hpp>

#include "UserInput/GLFW/GLFWUserInput.hpp"
#include <RHI/Vulkan/Framework/VulkanApp.hpp>
#include <RHI/Vulkan/Renderers/VulkanClear.hpp>
#include <RHI/Vulkan/Renderers/VulkanFinish.hpp>
#include <RHI/Vulkan/Renderers/VulkanPBRModelRenderer.hpp>

#include "Camera/TestCamera.hpp"

#include <Filesystem/FilesystemUtilities.hpp>

struct UBO
{
	mat4 mvp, mv, m;
	vec4 cameraPos;
} ubo;

Vulkan_GLTF_PBRRender::Vulkan_GLTF_PBRRender(const uint32_t screenWidth, uint32_t screenHeight)
	: VulkanBaseRender(screenWidth, screenHeight)
	, positioner_(new CameraPositioner_FirstPerson(vec3(0.0f, 6.0f, 11.0f), vec3(0.0f, 4.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, camera_(new TestCamera(*positioner_))
{
	window_ = initVulkanApp(screenWidth, screenHeight);

	input_ = &GLFWUserInput::GetInput();
	input_->positioner = positioner_;
}

void Vulkan_GLTF_PBRRender::updateBuffers(uint32_t imageIndex)
{
	int width, height;
	glfwGetFramebufferSize(window_, &width, &height);
	const float ratio = (float)width / (float)height;

	const mat4 scale = glm::scale(mat4(1.0f), vec3(5.0f));
	const mat4 rot1 = glm::rotate(mat4(1.0f), glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
	const mat4 rot2 = glm::rotate(mat4(1.0f), glm::radians(180.0f), vec3(0.0f, 0.0f, 1.0f));
	const mat4 rot = rot1 * rot2;
	const mat4 pos = glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, +1.0f));
	const mat4 m = glm::rotate(scale * rot * pos, (float)glfwGetTime() * -0.1f, vec3(0.0f, 0.0f, 1.0f));
	const mat4 proj = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
	const mat4 mvp = proj * camera_->getViewMatrix() * m;

	const mat4 mv = camera_->getViewMatrix() * m;

	ubo = UBO{ mvp, mv, m, vec4(camera_->getPosition(), 1.0f) };
	modelPBR->updateUniformBuffer(vkDev, imageIndex, &ubo, sizeof(ubo));
}

void Vulkan_GLTF_PBRRender::composeFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	clear->fillCommandBuffer(commandBuffer, imageIndex);
	modelPBR->fillCommandBuffer(commandBuffer, imageIndex);
	finish->fillCommandBuffer(commandBuffer, imageIndex);
}

bool Vulkan_GLTF_PBRRender::initVulkan()
{
	createInstance(&vk_.instance);

	BL_CHECK(setupDebugCallbacks(vk_.instance, &vk_.messenger, &vk_.reportCallback));

	VK_CHECK(glfwCreateWindowSurface(vk_.instance, window_, nullptr, &vk_.surface));

	BL_CHECK(initVulkanRenderDeviceWithCompute(vk_, vkDev, kScreenWidth, kScreenHeight, VkPhysicalDeviceFeatures{}));

	createDepthResources(vkDev, vkDev.framebufferWidth, vkDev.framebufferHeight, depthTexture);

	clear = std::make_unique<VulkanClear>(vkDev, depthTexture);
	finish = std::make_unique<VulkanFinish>(vkDev, depthTexture);

	modelPBR = std::make_unique<PBRModelRenderer>(vkDev,
		(uint32_t)sizeof(UBO),
		(FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/DamagedHelmet.gltf").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_AO.jpg").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_emissive.jpg").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_albedo.jpg").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_metalRoughness.jpg").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "objects/DamagedHelmet/glTF/Default_normal.jpg").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k.hdr").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k_irradiance.hdr").c_str(),
		depthTexture);

	return VK_SUCCESS;
}

void Vulkan_GLTF_PBRRender::terminateVulkan()
{
	clear = nullptr;
	finish = nullptr;
	modelPBR = nullptr;

	destroyVulkanImage(vkDev.device, depthTexture);

	destroyVulkanRenderDevice(vkDev);
	destroyVulkanInstance(vk_);
}

int Vulkan_GLTF_PBRRender::draw()
{
	initVulkan();

	double lastTime = glfwGetTime();

	do
	{
		drawFrame(vkDev, 
			[=](uint32_t imageIndex) { this->updateBuffers(imageIndex); },
			[=](VkCommandBuffer commandBuffer, uint32_t imageIndex) { this->composeFrame(commandBuffer, imageIndex); });

		const double newTime = glfwGetTime();
		const float deltaSeconds = static_cast<float>(newTime - lastTime);
		positioner_->update(deltaSeconds, input_->mouseState->pos, input_->mouseState->pressedLeft);
		lastTime = newTime;

		glfwPollEvents();
	} while (!glfwWindowShouldClose(window_));

	terminateVulkan();

	glfwTerminate();
	glslang_finalize_process();

	return 0;
}