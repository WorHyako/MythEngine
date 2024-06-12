#include <RHI/Vulkan/VulkanBRDFLUTRender.hpp>
#include <RHI/Vulkan/Framework/VulkanApp.hpp>
#include <RHI/Vulkan/Renderers/VulkanComputeBase.hpp>

#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <gli/load_ktx.hpp>

#include <Filesystem/FilesystemUtilities.hpp>

constexpr  int brdfW = 256;
constexpr  int brdfH = 256;

const uint32_t bufferSize = 2 * sizeof(float) * brdfW * brdfH;

float lutData[bufferSize];

void VulkanBRDFLUTRender::calculateLUT(float* output)
{
	ComputeBase cb(vkDev, (FilesystemUtilities::GetShadersDir() + "Vulkan/VK01_BRDF_LUT.comp").c_str(), sizeof(float), bufferSize);

	if (!cb.execute(brdfW, brdfH, 1))
		exit(EXIT_FAILURE);

	cb.downloadOutput(0, (uint8_t*)lutData, bufferSize);
}

gli::texture convertLUTtoTexture(const float* data)
{
	gli::texture lutTexture = gli::texture2d(gli::FORMAT_RG16_SFLOAT_PACK16, gli::extent2d(brdfW, brdfH), 1);

	for(int y = 0; y < brdfH; y++)
	{
		for(int x = 0; x < brdfW; x++)
		{
			const int ofs = y * brdfW + x;
			const gli::vec2 value(data[ofs * 2 + 0], data[ofs * 2 + 1]);
			const gli::texture::extent_type uv = { x, y , 0 };
			lutTexture.store<glm::uint32>(uv, 0, 0, 0, gli::packHalf2x16(value));
		}
	}

	return lutTexture;
}

VulkanBRDFLUTRender::VulkanBRDFLUTRender(const uint32_t screenWidth, uint32_t screenHeight)
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

	window_ = glfwCreateWindow(brdfW, brdfH, "VulkanApp", nullptr, nullptr);
	if (!window_)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
}

bool VulkanBRDFLUTRender::initVulkan()
{
	createInstance(&vk_.instance);

	BL_CHECK(setupDebugCallbacks(vk_.instance, &vk_.messenger, &vk_.reportCallback));

	VK_CHECK(glfwCreateWindowSurface(vk_.instance, window_, nullptr, &vk_.surface));

	BL_CHECK(initVulkanRenderDeviceWithCompute(vk_, vkDev, brdfW, brdfH, VkPhysicalDeviceFeatures{}));
	
	return VK_SUCCESS;
}

void VulkanBRDFLUTRender::terminateVulkan()
{
	destroyVulkanRenderDevice(vkDev);
	destroyVulkanInstance(vk_);
}

int VulkanBRDFLUTRender::draw()
{
	initVulkan();

	printf("Calculating LUT texture...\n");
	calculateLUT(lutData);

	printf("Saving LUT texture...\n");
	gli::texture lutTexture = convertLUTtoTexture(lutData);

	// use Pico Pixel to view https://pixelandpolygon.com/
	gli::save_ktx(lutTexture, (FilesystemUtilities::GetResourcesDir() + "Data/brdfLUT.ktx").c_str());

	terminateVulkan();

	glfwTerminate();
	glslang_finalize_process();

	return 0;
}