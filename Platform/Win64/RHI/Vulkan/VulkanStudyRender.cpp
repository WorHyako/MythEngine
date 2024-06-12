#include <RHI/Vulkan/VulkanStudyRender.hpp>

#ifndef __gl_h_
#include <glad/gl.h>
#endif
#define VK_NO_PROTOTYPES
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#define VOLK_IMPLEMENTATION
#include <volk.h>

#include <imgui.h>
#include <cstdio>

#include <Utils/Utils.hpp>
#include <Utils/UtilsFPS.hpp>
#include <RHI/Vulkan/UtilsVulkan.hpp>
#include <RHI/Vulkan/Renderers/VulkanCanvas.hpp>
#include <RHI/Vulkan/Renderers/VulkanCube.hpp>
#include <RHI/Vulkan/Renderers/VulkanImGui.hpp>
#include <RHI/Vulkan/Renderers/VulkanClear.hpp>
#include <RHI/Vulkan/Renderers/VulkanFinish.hpp>
#include <RHI/Vulkan/Renderers/VulkanModelRenderer.hpp>
#include <RHI/Vulkan/Renderers/VulkanMultiMeshRenderer.hpp>
#include <Camera/TestCamera.hpp>
#include <EasyProfilerWrapper.hpp>
#include <Graph.hpp>
#include <UserInput/GLFW/GLFWUserInput.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <Filesystem/FilesystemUtilities.hpp>

using glm::mat4;
using glm::vec3;
using glm::vec4;

std::unique_ptr<ImGuiRenderer> imgui;
std::unique_ptr<ModelRenderer> modelRenderer;
std::unique_ptr<MultiMeshRenderer> multiRenderer;
std::unique_ptr<CubeRenderer> cubeRenderer;
std::unique_ptr<VulkanCanvas> canvas;
std::unique_ptr<VulkanCanvas> canvas2d;
std::unique_ptr<VulkanClear> clear;
std::unique_ptr<VulkanFinish> finish;

FramesPerSecondCounter fpsCounter(0.02f);
LinearGraph fpsGraph;
LinearGraph sineGraph(4096);

glm::vec3 vulkanCameraPos(0.0f, 0.0f, 0.0f);
glm::vec3 cameraAngles(-45.0f, 0.0f, 0.0f);

CameraPositioner_FirstPerson positioner_firstPerson(vulkanCameraPos, vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));
CameraPositioner_MoveTo positioner_moveTo(vulkanCameraPos, cameraAngles);
TestCamera camera = TestCamera(positioner_firstPerson);

// ImGUI stuff
const char* cameraType = "FirstPerson";
const char* comboBoxItems[] = { "FirstPerson", "MoveTo" };
const char* currentComboBoxItem = cameraType;

void saveSPIRVBinaryFile(const char* filename, unsigned int* code, size_t size)
{
	FILE* f = fopen(filename, "w");

	if(!f)
		return;

	fwrite(code, sizeof(uint32_t), size, f);
	fclose(f);
}

void testShaderCompilation(const char* sourceFileName, const char* destFileName)
{
	ShaderModule shaderModule;

	if(compileShaderFile(sourceFileName, shaderModule) < 1)
		return;

	saveSPIRVBinaryFile(destFileName, shaderModule.SPIRV.data(), shaderModule.SPIRV.size());
}

bool VulkanStudyRender::initVulkan()
{
	EASY_FUNCTION();

	createInstance(&vk_.instance);

	if (!setupDebugCallbacks(vk_.instance, &vk_.messenger, &vk_.reportCallback))
		exit(EXIT_FAILURE);

	if (glfwCreateWindowSurface(vk_.instance, window_, nullptr, &vk_.surface))
		exit(EXIT_FAILURE);

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.geometryShader = VK_TRUE;
    deviceFeatures.multiDrawIndirect = VK_TRUE;
    deviceFeatures.drawIndirectFirstInstance = VK_TRUE;

	if (!initVulkanRenderDevice(vk_, vkDev, kScreenWidth, kScreenHeight, isDeviceSuitable, deviceFeatures))
		exit(EXIT_FAILURE);

	imgui = std::make_unique<ImGuiRenderer>(vkDev);
	modelRenderer = std::make_unique<ModelRenderer>(vkDev, (FilesystemUtilities::GetResourcesDir() + "objects/rubber_duck/scene.gltf").c_str(), (FilesystemUtilities::GetResourcesDir() + "objects/rubber_duck/textures/Duck_baseColor.png").c_str(), (uint32_t)sizeof(glm::mat4));
    multiRenderer = std::make_unique<MultiMeshRenderer>(vkDev, 
		(FilesystemUtilities::GetResourcesDir() + "Meshes/test.meshes").c_str(), 
		(FilesystemUtilities::GetResourcesDir() + "Meshes/test.meshes.drawdata").c_str(), 
		"",
		(FilesystemUtilities::GetShadersDir() + "Vulkan/VKIndirect01.vert").c_str(),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/VKIndirect01.frag").c_str());
	cubeRenderer = std::make_unique<CubeRenderer>(vkDev, modelRenderer->getDepthTexture(), (FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k.hdr").c_str());
	clear = std::make_unique<VulkanClear>(vkDev, modelRenderer->getDepthTexture());
	finish = std::make_unique<VulkanFinish>(vkDev, modelRenderer->getDepthTexture());
	canvas2d = std::make_unique<VulkanCanvas>(vkDev, VulkanImage{ VK_NULL_HANDLE, VK_NULL_HANDLE });
	canvas = std::make_unique<VulkanCanvas>(vkDev, modelRenderer->getDepthTexture());

	return true;
}

void VulkanStudyRender::terminateVulkan()
{
	canvas = nullptr;
	canvas2d = nullptr;
	finish = nullptr;
	clear = nullptr;
	cubeRenderer = nullptr;
	modelRenderer = nullptr;
    multiRenderer = nullptr;
	imgui = nullptr;
	destroyVulkanRenderDevice(vkDev);
	destroyVulkanInstance(vk_);
}

void reinitCamera()
{
	if(!strcmp(cameraType, "FirstPerson"))
	{
		camera = TestCamera(positioner_firstPerson);
	}
	else
	{
		if(!strcmp(cameraType, "MoveTo"))
		{
			positioner_moveTo.setDesiredPosition(vulkanCameraPos);
			positioner_moveTo.setDesiredAngles(cameraAngles.x, cameraAngles.y, cameraAngles.z);
			camera = TestCamera(positioner_moveTo);
		}
	}
}

void VulkanStudyRender::renderGUI(uint32_t imageIndex)
{
	EASY_FUNCTION();

	int width, height;
	glfwGetFramebufferSize(window_, &width, &height);

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)width, (float)height);
	ImGui::NewFrame();

	const ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoBackground;

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("Statistics", nullptr, flags);
	ImGui::Text("FPS: %.2f", fpsCounter.getFPS());
	ImGui::End();

	ImGui::Begin("Camera Control", nullptr);
	{
		if(ImGui::BeginCombo("##combo", currentComboBoxItem)) // The second parameter is the label previewed before opening the combo.
		{
			for(int n = 0; n < IM_ARRAYSIZE(comboBoxItems); n++)
			{
				const bool isSelected = (currentComboBoxItem == comboBoxItems[n]);

				if (ImGui::Selectable(comboBoxItems[n], isSelected))
					currentComboBoxItem = comboBoxItems[n];

				if (isSelected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}

		if(!strcmp(cameraType, "MoveTo"))
		{
			if (ImGui::SliderFloat3("Position", glm::value_ptr(vulkanCameraPos), -10.0f, +10.0f))
				positioner_moveTo.setDesiredPosition(vulkanCameraPos);
			if (ImGui::SliderFloat3("Pitch/Pan/Roll", glm::value_ptr(cameraAngles), -90.0f, +90.0f))
				positioner_moveTo.setDesiredAngles(cameraAngles);
		}

		if(currentComboBoxItem && strcmp(currentComboBoxItem, cameraType))
		{
			printf("Selected new camera type: %s\n", currentComboBoxItem);
			cameraType = currentComboBoxItem;
			reinitCamera();
		}
	}
	ImGui::End();
	ImGui::Render();

	imgui->updateBuffers(vkDev, imageIndex, ImGui::GetDrawData());
}

void VulkanStudyRender::update3D(uint32_t imageIndex)
{
	int width, height;
	glfwGetFramebufferSize(window_, &width, &height);
	const float ratio = width / (float)height;

	const mat4 m1 = glm::rotate(glm::translate(mat4(1.0f), vec3(0.f, 0.5f, -1.5f)) * glm::rotate(mat4(1.f), glm::pi<float>(), vec3(1, 0, 0)), (float)glfwGetTime(), vec3(0.0f, 1.0f, 0.0f));
	const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

	const mat4 view = camera.getViewMatrix();
	const mat4 mtx = p * view * m1;

	{
		EASY_BLOCK("UpdateUniformBuffers");
		modelRenderer->updateUniformBuffer(vkDev, imageIndex, glm::value_ptr(mtx), sizeof(mat4));
        multiRenderer->updateUniformBuffer(vkDev, imageIndex, mtx);
		canvas->updateUniformBuffer(vkDev, p * view, 0.0f, imageIndex);
		canvas2d->updateUniformBuffer(vkDev, glm::ortho(0, 1, 1, 0), 0.0f, imageIndex);
		cubeRenderer->updateUniformBuffer(vkDev, imageIndex, p * view * m1);
		EASY_END_BLOCK;
	}
}

void VulkanStudyRender::update2D(uint32_t imageIndex)
{
	canvas2d->clear();
	sineGraph.renderGraph(*canvas2d.get(), vec4(0.0f, 1.0f, 0.0f, 1.0f));
	fpsGraph.renderGraph(*canvas2d.get());
	canvas2d->updateBuffer(vkDev, imageIndex);
}

void VulkanStudyRender::composeFrame(uint32_t imageIndex, const std::vector<RendererBase*>&renderers)
{
	update3D(imageIndex);
	renderGUI(imageIndex);
	update2D(imageIndex);

	EASY_BLOCK("FillCommandBuffers");

	VkCommandBuffer commandBuffer = vkDev.commandBuffers[imageIndex];

	VkCommandBufferBeginInfo bi{};
	bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bi.pNext = nullptr;
	bi.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	bi.pInheritanceInfo = nullptr;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));

	for (auto& r : renderers)
		r->fillCommandBuffer(commandBuffer, imageIndex);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	EASY_END_BLOCK;
}

bool VulkanStudyRender::drawFrame(const std::vector<RendererBase*>& renderers)
{
	EASY_FUNCTION();

	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(vkDev.device, vkDev.swapchain, 0, vkDev.semaphore, VK_NULL_HANDLE, &imageIndex);
	VK_CHECK(vkResetCommandPool(vkDev.device, vkDev.commandPool, 0));

	if (result != VK_SUCCESS) return false;

	composeFrame(imageIndex, renderers);

	const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // or even VERTEX_SHADER_STAGE

	VkSubmitInfo si{};
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	si.pNext = nullptr;
	si.waitSemaphoreCount = 1;
	si.pWaitSemaphores = &vkDev.semaphore;
	si.pWaitDstStageMask = waitStages;
	si.commandBufferCount = 1;
	si.pCommandBuffers = &vkDev.commandBuffers[imageIndex];
	si.signalSemaphoreCount = 1;
	si.pSignalSemaphores = &vkDev.renderSemaphore;

	{
		EASY_BLOCK("vkQueueSubmit", profiler::colors::Magenta);
		VK_CHECK(vkQueueSubmit(vkDev.graphicsQueue, 1, &si, nullptr));
		EASY_END_BLOCK;
	}

	VkPresentInfoKHR pi{};
	pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pi.pNext = nullptr;
	pi.waitSemaphoreCount = 1;
	pi.pWaitSemaphores = &vkDev.renderSemaphore;
	pi.swapchainCount = 1;
	pi.pSwapchains = &vkDev.swapchain;
	pi.pImageIndices = &imageIndex;

	{
		EASY_BLOCK("vkQueuePresentKHR", profiler::colors::Magenta);
		VK_CHECK(vkQueuePresentKHR(vkDev.graphicsQueue, &pi));
		EASY_END_BLOCK;
	}

	{
		EASY_BLOCK("vkDeviceWaitIdle", profiler::colors::Red);
		VK_CHECK(vkDeviceWaitIdle(vkDev.device));
		EASY_END_BLOCK;
	}

	return true;
}

/*
This program should give the same result as the following commands:

	glslangValidator -V110 --target-env spirv1.3 VK01.vert -o VK01.vert.bin
	glslangValidator -V110 --target-env spirv1.3 VK01.frag -o VK01.frag.bin
*/

VulkanStudyRender::VulkanStudyRender(const uint32_t screenWidth, uint32_t screenHeight)
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

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	window_ = glfwCreateWindow(kScreenWidth, kScreenHeight, "VulkanApp", nullptr, nullptr);
	if (!window_)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	initVulkan();
	
	input_ = &GLFWUserInput::GetInput();
	input_->positioner = &positioner_firstPerson;
}

int VulkanStudyRender::draw()
{
	EASY_PROFILER_ENABLE;
	EASY_MAIN_THREAD;

	{
		canvas->plane3d(vec3(0, +1.5, 0), vec3(1, 0, 0), vec3(0, 0, 1), 40, 40, 10.0f, 10.0f, vec4(1, 0, 0, 1), vec4(0, 1, 0, 1));

		for (size_t i = 0; i < vkDev.swapchainImages.size(); i++)
			canvas->updateBuffer(vkDev, i);
	}

	testShaderCompilation((FilesystemUtilities::GetShadersDir() + "Vulkan/VK01.vert").c_str(), "VK01.vrt.bin");
	testShaderCompilation((FilesystemUtilities::GetShadersDir() + "Vulkan/VK01.frag").c_str(), "VK01.frg.bin");

	double timeStamp = glfwGetTime();
	float deltaSeconds = 0.0f;

	const std::vector<RendererBase*> renderers = { clear.get(), cubeRenderer.get(), modelRenderer.get(), multiRenderer.get(), canvas.get(), canvas2d.get(), imgui.get(), finish.get() };

	while(!glfwWindowShouldClose(window_))
	{
		{
			EASY_BLOCK("UpdateCameraPositioners");
			positioner_firstPerson.update(deltaSeconds, input_->mouseState->pos, input_->mouseState->pressedLeft);
			positioner_moveTo.update(deltaSeconds, input_->mouseState->pos, input_->mouseState->pressedLeft);
			EASY_END_BLOCK;
		}

		const double newTimeStamp = glfwGetTime();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		const bool frameRendered = drawFrame(renderers);

		if(fpsCounter.tick(deltaSeconds, frameRendered))
		{
			fpsGraph.addPoint(fpsCounter.getFPS());
		}
		sineGraph.addPoint((float)sin(glfwGetTime() * 10.0));

		{
			EASY_BLOCK("PoolEvents");
			glfwPollEvents();
			EASY_END_BLOCK;
		}
	}

	ImGui::DestroyContext();

	terminateVulkan();
	glfwTerminate();
	glslang_finalize_process();

	PROFILER_DUMP("profiling.prof");

	return 0;
}