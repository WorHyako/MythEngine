#pragma once

#include <RHI/Vulkan/Framework/CompositeRenderer.hpp>
#include <RHI/Vulkan/Framework/VulkanShaderProcessor.hpp>
#include <RHI/Vulkan/Framework/Barriers.hpp>

const VkFormat LuminosityFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

const int LuminosityWidth = 64;
const int LuminosityHeight = 64;

struct LuminanceCalculator : public CompositeRenderer
{
	LuminanceCalculator(VulkanRenderContext& ctx, VulkanTexture sourceTex, VulkanTexture lumTex)
		: CompositeRenderer(ctx)
		, source(sourceTex)

		, lumTex64(ctx.resources.addColorTexture(LuminosityWidth, LuminosityHeight, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, lumTex32(ctx.resources.addColorTexture(LuminosityWidth / 2, LuminosityHeight / 2, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, lumTex16(ctx.resources.addColorTexture(LuminosityWidth / 4, LuminosityHeight / 4, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, lumTex08(ctx.resources.addColorTexture(LuminosityWidth / 8, LuminosityHeight / 8, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, lumTex04(ctx.resources.addColorTexture(LuminosityWidth / 16, LuminosityHeight / 16, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, lumTex02(ctx.resources.addColorTexture(LuminosityWidth / 32, LuminosityHeight / 32, LuminosityFormat, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE))
		, lumTex01(lumTex)

		, src_To_64(ctx, DescriptorSetInfo{
			{}, {fsTextureAttachment(source)}},
			{lumTex64},
			(FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Downscale2x2.frag").c_str())
		, lum64_To_32(ctx, DescriptorSetInfo{
			{}, {fsTextureAttachment(lumTex64)}},
			{ lumTex32 },
			(FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Downscale2x2.frag").c_str())
		, lum32_To_16(ctx, DescriptorSetInfo{
			{}, {fsTextureAttachment(lumTex32)}},
			{ lumTex16 },
			(FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Downscale2x2.frag").c_str())
		, lum16_To_08(ctx, DescriptorSetInfo{
			{}, {fsTextureAttachment(lumTex16)} },
			{ lumTex08 },
			(FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Downscale2x2.frag").c_str())
		, lum08_To_04(ctx, DescriptorSetInfo{
			{}, {fsTextureAttachment(lumTex08)} },
			{ lumTex04 },
			(FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Downscale2x2.frag").c_str())
		, lum04_To_02(ctx, DescriptorSetInfo{
			{}, {fsTextureAttachment(lumTex04)} },
			{ lumTex02 },
			(FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Downscale2x2.frag").c_str())
		, lum02_To_01(ctx, DescriptorSetInfo{
			{}, {fsTextureAttachment(lumTex02)} },
			{ lumTex01 },
			(FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/Downscale2x2.frag").c_str())

		, lum64ToColor(ctx, lumTex64)
		, lum64ToShader(ctx, lumTex64)

		, lum32ToColor(ctx, lumTex32)
		, lum32ToShader(ctx, lumTex32)

		, lum16ToColor(ctx, lumTex16)
		, lum16ToShader(ctx, lumTex16)

		, lum08ToColor(ctx, lumTex08)
		, lum08ToShader(ctx, lumTex08)

		, lum04ToColor(ctx, lumTex04)
		, lum04ToShader(ctx, lumTex04)

		, lum02ToColor(ctx, lumTex02)
		, lum02ToShader(ctx, lumTex02)

		, lum01ToColor(ctx, lumTex01)
		, lum01ToShader(ctx, lumTex01)
	{
		setVkImageName(ctx.vkDev, lumTex64.image.image, "lum64");
		setVkImageName(ctx.vkDev, lumTex32.image.image, "lum32");
		setVkImageName(ctx.vkDev, lumTex16.image.image, "lum16");
		setVkImageName(ctx.vkDev, lumTex08.image.image, "lum08");
		setVkImageName(ctx.vkDev, lumTex04.image.image, "lum04");
		setVkImageName(ctx.vkDev, lumTex02.image.image, "lum02");

		renderers_.emplace_back(lum64ToColor, false);
		renderers_.emplace_back(src_To_64, false);
		renderers_.emplace_back(lum64ToShader, false);

		renderers_.emplace_back(lum32ToColor, false);
		renderers_.emplace_back(lum64_To_32, false);
		renderers_.emplace_back(lum32ToShader, false);

		renderers_.emplace_back(lum16ToColor, false);
		renderers_.emplace_back(lum32_To_16, false);
		renderers_.emplace_back(lum16ToShader, false);

		renderers_.emplace_back(lum08ToColor, false);
		renderers_.emplace_back(lum16_To_08, false);
		renderers_.emplace_back(lum08ToShader, false);

		renderers_.emplace_back(lum04ToColor, false);
		renderers_.emplace_back(lum08_To_04, false);
		renderers_.emplace_back(lum04ToShader, false);

		renderers_.emplace_back(lum02ToColor, false);
		renderers_.emplace_back(lum04_To_02, false);
		renderers_.emplace_back(lum02ToShader, false);

		renderers_.emplace_back(lum01ToColor, false);
		renderers_.emplace_back(lum02_To_01, false);
		renderers_.emplace_back(lum01ToShader, false);
	}

	inline VulkanTexture getResult64() const { return lumTex64; }
	inline VulkanTexture getResult32() const { return lumTex32; }
	inline VulkanTexture getResult16() const { return lumTex16; }
	inline VulkanTexture getResult08() const { return lumTex08; }
	inline VulkanTexture getResult04() const { return lumTex04; }
	inline VulkanTexture getResult02() const { return lumTex02; }
	inline VulkanTexture getResult01() const { return lumTex01; }

private:
	VulkanTexture source;

	VulkanTexture lumTex64;
	VulkanTexture lumTex32;
	VulkanTexture lumTex16;
	VulkanTexture lumTex08;
	VulkanTexture lumTex04;
	VulkanTexture lumTex02;
	VulkanTexture lumTex01;

	QuadProcessor src_To_64;
	QuadProcessor lum64_To_32;
	QuadProcessor lum32_To_16;
	QuadProcessor lum16_To_08;
	QuadProcessor lum08_To_04;
	QuadProcessor lum04_To_02;
	QuadProcessor lum02_To_01;

	ShaderOptimalToColorBarrier lum64ToColor;
	ColorToShaderOptimalBarrier lum64ToShader;
	ShaderOptimalToColorBarrier lum32ToColor;
	ColorToShaderOptimalBarrier lum32ToShader;
	ShaderOptimalToColorBarrier lum16ToColor;
	ColorToShaderOptimalBarrier lum16ToShader;
	ShaderOptimalToColorBarrier lum08ToColor;
	ColorToShaderOptimalBarrier lum08ToShader;
	ShaderOptimalToColorBarrier lum04ToColor;
	ColorToShaderOptimalBarrier lum04ToShader;
	ShaderOptimalToColorBarrier lum02ToColor;
	ColorToShaderOptimalBarrier lum02ToShader;
	ShaderOptimalToColorBarrier lum01ToColor;
	ColorToShaderOptimalBarrier lum01ToShader;
};