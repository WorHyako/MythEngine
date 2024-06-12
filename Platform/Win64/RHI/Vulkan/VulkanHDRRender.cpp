#include <RHI/Vulkan/VulkanHDRRender.hpp>

const uint32_t TEX_RGB = (0x2 << 16);

HDRApp::HDRApp()
	: CameraApp(-95, -95)
	, cubemap(ctx_.resources.loadCubemap((FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k.hdr").c_str()))
	, cubemapIrr(ctx_.resources.loadCubemap((FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k_irradiance.hdr").c_str()))

	, HDRDepth(ctx_.resources.addDepthTexture())
	, HDRLuminance(ctx_.resources.addColorTexture(0, 0, LuminosityFormat))
	, luminanceResult(ctx_.resources.addColorTexture(1, 1, LuminosityFormat))

	, hdrTex(ctx_.resources.addColorTexture())

	, sceneData(ctx_,
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.materials").c_str(),
		cubemap, cubemapIrr, true)

	, cubeRenderer(ctx_, cubemap, {HDRLuminance, HDRDepth},
		ctx_.resources.addRenderPass({HDRLuminance, HDRDepth}, 
		RenderPassCreateInfo{true, true, eRenderPassBit_First | eRenderPassBit_Offscreen}))

	, multiRenderer(ctx_, sceneData,
		(FilesystemUtilities::GetShadersDir() + "Vulkan/MultiRenderer/MultiRenderer.vert").c_str(),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/HDR/SceneIBL.frag").c_str(),
		{ HDRLuminance, HDRDepth },
		ctx_.resources.addRenderPass({ HDRLuminance, HDRDepth },
			RenderPassCreateInfo{
				false, false, eRenderPassBit_Offscreen
			}))

	// tone mapping (gamma correction / exposure)
	, luminance(ctx_, HDRLuminance, luminanceResult)
	// Temporarily we switch between luminances [coming from PingPong light adaptation calculator]
	, hdr(ctx_, HDRLuminance, luminanceResult, mappedUniformBufferAttachment(ctx_.resources, &hdrUniforms, VK_SHADER_STAGE_FRAGMENT_BIT))

	, displayedTextureList(
		{ hdrTex,
		   HDRLuminance, luminance.getResult64(),
		   luminance.getResult32(), luminance.getResult16(), luminance.getResult08(),
		   luminance.getResult04(), luminance.getResult02(), luminance.getResult01(), // 2 - 9
		   hdr.getBloom1(), hdr.getBloom2(), hdr.getBrightness(), hdr.getResult(), // 10 - 13
		   HDRLuminance, hdr.getStreaks1(), hdr.getStreaks2(),  // 14 - 16
		   hdr.getAdaptatedLum1(), hdr.getAdaptatedLum2() })// 17 - 18

	, quads(ctx_, displayedTextureList)
	, imgui(ctx_, displayedTextureList)

	, toDepth(ctx_, HDRDepth)
	, toShader(ctx_, HDRDepth)

	, lumToColor(ctx_, HDRLuminance)
	, lumToShader(ctx_, HDRLuminance)

	, lumWait(ctx_, HDRLuminance)
{
	positioner = CameraPositioner_FirstPerson(vec3(-15.81f, -5.18f, -5.81f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));

	hdrUniforms->bloomStrength = 1.1f;
	hdrUniforms->maxWhite = 1.17f;
	hdrUniforms->exposure = 0.9f;
	hdrUniforms->adaptationSpeed = 0.1f;

	setVkImageName(ctx_.vkDev, HDRDepth.image.image, "HDRDepth");
	setVkImageName(ctx_.vkDev, HDRLuminance.image.image, "HDRLuminance");
	setVkImageName(ctx_.vkDev, hdrTex.image.image, "hdrTex");
	setVkImageName(ctx_.vkDev, luminanceResult.image.image, "lumRes");

	setVkImageName(ctx_.vkDev, hdr.getBloom1().image.image, "bloom1");
	setVkImageName(ctx_.vkDev, hdr.getBloom2().image.image, "bloom2");
	setVkImageName(ctx_.vkDev, hdr.getBrightness().image.image, "bloomBright");
	setVkImageName(ctx_.vkDev, hdr.getResult().image.image, "bloomResult");
	setVkImageName(ctx_.vkDev, hdr.getStreaks1().image.image, "bloomStreaks1");
	setVkImageName(ctx_.vkDev, hdr.getStreaks2().image.image, "bloomStreaks2");

	onScreenRenderers_.emplace_back(cubeRenderer);

	onScreenRenderers_.emplace_back(toDepth, false);
	onScreenRenderers_.emplace_back(lumToColor, false);

	onScreenRenderers_.emplace_back(multiRenderer);

	onScreenRenderers_.emplace_back(lumWait, false);

	onScreenRenderers_.emplace_back(luminance, false);

	onScreenRenderers_.emplace_back(hdr, false);

	onScreenRenderers_.emplace_back(quads, false);
	onScreenRenderers_.emplace_back(imgui, false);
}

void HDRApp::drawUI()
{
	ImGui::Begin("Settings", nullptr);
		ImGui::Text("FPS: %.2f", getFPS());

		ImGui::Checkbox("Enable ToneMapping", &enableToneMapping);
		ImGui::Checkbox("ShowPyramid", &showPyramid);
		ImGui::Checkbox("ShowDebug", &showDebug);

		ImGui::SliderFloat("BloomStrength: ", &hdrUniforms->bloomStrength, 0.1f, 2.0f);
		ImGui::SliderFloat("MaxWhite: ", &hdrUniforms->maxWhite, 0.1f, 2.0f);
		ImGui::SliderFloat("Exposure: ", &hdrUniforms->exposure, 0.1f, 10.0f);
		ImGui::SliderFloat("Adaptation speed: ", &hdrUniforms->adaptationSpeed, 0.01f, 2.0f);
	ImGui::End();

	if(showPyramid)
	{
		ImGui::Begin("Pyramid", nullptr);

		ImGui::Text("HDRColor");
		ImGui::Image((void*)(intptr_t)(2 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Lum64");
		ImGui::Image((void*)(intptr_t)(3 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Lum32");
		ImGui::Image((void*)(intptr_t)(4 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Lum16");
		ImGui::Image((void*)(intptr_t)(5 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Lum08");
		ImGui::Image((void*)(intptr_t)(6 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Lum04");
		ImGui::Image((void*)(intptr_t)(7 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Lum02");
		ImGui::Image((void*)(intptr_t)(8 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Lum01");
		ImGui::Image((void*)(intptr_t)(9 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::End();
	}

	if(showDebug)
	{
		ImGui::Begin("Adaptation", nullptr);
		ImGui::Text("Adapt1");
		ImGui::Image((void*)(intptr_t)(17 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Adapt2");
		ImGui::Image((void*)(intptr_t)(18 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::End();

		ImGui::Begin("Debug", nullptr);
		ImGui::Text("Bloom1");
		ImGui::Image((void*)(intptr_t)(10 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Bloom2");
		ImGui::Image((void*)(intptr_t)(11 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Bright");
		ImGui::Image((void*)(intptr_t)(12 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Result");
		ImGui::Image((void*)(intptr_t)(13 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

		ImGui::Text("Streaks1");
		ImGui::Image((void*)(intptr_t)(14 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Streaks2");
		ImGui::Image((void*)(intptr_t)(15 | TEX_RGB), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::End();
	}
}

void HDRApp::draw3D()
{
	const mat4 p = getDefaultProjection();
	const mat4 view = camera.getViewMatrix();
	cubeRenderer.setMatrices(p, view);
	multiRenderer.setMatrices(p, view);

	multiRenderer.checkLoadedTextures();

	quads.clear();
	quads.quad(-1.0f, 1.0f, 1.0f, -1.0f, 12);
}
