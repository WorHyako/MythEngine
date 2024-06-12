#include <RHI/Vulkan/VulkanSSAORender.hpp>

#include <imgui_internal.h>

const char* envMapFile = "textures/piazza_bologni_1k.hdr";
const char* irrMapFile = "textures/piazza_bologni_1k_irradiance.hdr";

SSAOApp::SSAOApp()
	: CameraApp(-95, -95)
	, colorTex(ctx_.resources.addColorTexture())
	, depthTex(ctx_.resources.addDepthTexture())
	, finalTex(ctx_.resources.addColorTexture())

	, sceneData(ctx_,
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.materials").c_str(),
		ctx_.resources.loadCubemap((FilesystemUtilities::GetResourcesDir() + envMapFile).c_str()),
		ctx_.resources.loadCubemap((FilesystemUtilities::GetResourcesDir() + irrMapFile).c_str()))

	, multiRenderer(ctx_, sceneData,
		(FilesystemUtilities::GetShadersDir() + "Vulkan/MultiRenderer/MultiRenderer.vert").c_str(),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/MultiRenderer/MultiRenderer.frag").c_str(),
		{colorTex, depthTex},
		ctx_.resources.addRenderPass({ colorTex, depthTex },
		RenderPassCreateInfo{
			true, true, eRenderPassBit_First | eRenderPassBit_Offscreen
		}))

	, SSAO(ctx_, colorTex, depthTex, finalTex)

	, quads(ctx_, {colorTex, finalTex})
	, imgui(ctx_, {colorTex, SSAO.getBlurY()})
{
	positioner = CameraPositioner_FirstPerson(glm::vec3(-10.0f, -3.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));

	onScreenRenderers_.emplace_back(multiRenderer);
	onScreenRenderers_.emplace_back(SSAO);

	onScreenRenderers_.emplace_back(quads, false);
	onScreenRenderers_.emplace_back(imgui, false);
}

void SSAOApp::drawUI()
{
	ImGui::Begin("Control", nullptr);
		ImGui::Checkbox("Enable SSAO", &enableSSAO);
		// https://github.com/ocornut/imgui/issues/1889#issuecomment-398681105
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !enableSSAO);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * enableSSAO ? 1.0f : 0.2f);

		ImGui::SliderFloat("SSAO scale", &SSAO.params->scale_, 0.0f, 2.0f);
		ImGui::SliderFloat("SSAO bias", &SSAO.params->bias_, 0.0f, 0.3f);
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	ImGui::Separator();
		ImGui::SliderFloat("SSAO radius", &SSAO.params->radius, 0.05f, 0.5f);
		ImGui::SliderFloat("SSAO attenuation scale", &SSAO.params->attScale, 0.5f, 1.5f);
		ImGui::SliderFloat("SSAO distance scale", &SSAO.params->distScale, 0.0f, 1.0f);
	ImGui::End();

	if(enableSSAO)
	{
		imguiTextureWindow("SSAO", 2);
	}
}

void SSAOApp::draw3D()
{
	multiRenderer.setMatrices(getDefaultProjection(), camera.getViewMatrix());

	quads.clear();
	quads.quad(-1.0f, -1.0f, 1.0f, 1.0f, enableSSAO ? 1 : 0);
}
