#include <RHI/Vulkan/VulkanShadowMappingRender.hpp>

namespace VulkanShadowMapping
{
	bool g_RotateModel = true;

	float g_ModelAngle = 0.0f;

	float g_LightAngle = 60.0f;
	float g_LightNear = 1.0f;
	float g_LightFar = 100.0f;

	float g_LightDist = 50.0f;
	float g_LightXAngle = -0.5f;
	float g_LightYAngle = 0.55f;

	const char* g_meshFile = "objects/rubber_duck/scene.gltf";
	const float g_meshScale = 20.0f; // scale from shader
}

struct Uniforms
{
	mat4 mvp;
	mat4 model;
	mat4 shadowMatrix;
	vec4 cameraPos;
	vec4 lightPos;
	float meshScale;
	float effects;
};

static_assert(sizeof(Uniforms) == sizeof(float) * 58);

ShadowMappingApp::ShadowMappingApp()
	: CameraApp(-95, -95)
	, meshBuffer(ctx_.resources.loadMeshToBuffer((FilesystemUtilities::GetResourcesDir() + VulkanShadowMapping::g_meshFile).c_str(), true, true, meshVertices, meshIndices))
	, planeBuffer(ctx_.resources.createPlaneBuffer_XY(2.0f, 2.0f))

	, meshUniformBuffer(ctx_.resources.addUniformBuffer(sizeof(Uniforms)))
	, shadowUniformBuffer(ctx_.resources.addUniformBuffer(sizeof(Uniforms)))

	, meshDepth(ctx_.resources.addDepthTexture())
	, meshColor(ctx_.resources.addColorTexture())

	, meshShadowDepth(ctx_.resources.addDepthTexture())
	, meshShadowColor(ctx_.resources.addColorTexture())

	, meshRenderer(ctx_, meshUniformBuffer, meshBuffer,
		{
			fsTextureAttachment(meshShadowDepth),
			fsTextureAttachment(ctx_.resources.loadTexture2D((FilesystemUtilities::GetResourcesDir() + "objects/rubber_duck/textures/Duck_baseColor.png").c_str()))
		},
		{meshColor, meshDepth},
		{
			(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/SceneModel.vert").c_str(),
			(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/SceneModel.frag").c_str()
		})

	, depthRenderer(ctx_, shadowUniformBuffer, meshBuffer, {},
		{meshShadowColor, meshShadowDepth},
		{
			(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/ShadowMapping.vert").c_str(),
			(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/ShadowMapping.frag").c_str()
		}
		, true)

	, planeRenderer(ctx_, meshUniformBuffer, planeBuffer,
		{
			fsTextureAttachment(meshShadowDepth),
			fsTextureAttachment(ctx_.resources.loadTexture2D((FilesystemUtilities::GetResourcesDir() + "textures/ch2_sample3_STB.jpg").c_str()))
		},
		{meshColor, meshDepth},
		{
			(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/SceneModel.vert").c_str(),
			(FilesystemUtilities::GetShadersDir() + "Vulkan/ShadowMapping/SceneModel.frag").c_str()
		})
	, canvas(ctx_, {meshColor, meshDepth}, true)

	, quads(ctx_, {meshColor, meshDepth, meshShadowColor, meshShadowDepth})
	, imgui(ctx_, {meshShadowColor, meshShadowDepth})
{
	onScreenRenderers_.emplace_back(canvas);
	onScreenRenderers_.emplace_back(meshRenderer);
	onScreenRenderers_.emplace_back(depthRenderer);
	onScreenRenderers_.emplace_back(planeRenderer);

	onScreenRenderers_.emplace_back(quads, false);
	onScreenRenderers_.emplace_back(imgui, false);

	positioner.lookAt(glm::vec3(-85.0f, 85.0f, 85.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));

	printf("Verts: %d\n", (int)meshVertices.size() / 8);

	canvas.clear();
}

void ShadowMappingApp::update(float deltaSeconds)
{
	if (VulkanShadowMapping::g_RotateModel)
		VulkanShadowMapping::g_ModelAngle += deltaSeconds;

	while (VulkanShadowMapping::g_ModelAngle > 360.0f)
		VulkanShadowMapping::g_ModelAngle -= 360.0f;
}

void ShadowMappingApp::drawUI()
{
	ImGui::Begin("Control", nullptr);
	ImGui::Checkbox("Rotate model", &VulkanShadowMapping::g_RotateModel);
	ImGui::Text("Light parameters", nullptr);
	ImGui::SliderFloat("Proj::Angle", &VulkanShadowMapping::g_LightAngle, 10.f, 170.0f);
	ImGui::SliderFloat("Proj::Near", &VulkanShadowMapping::g_LightNear, 0.1f, 5.0f);
	ImGui::SliderFloat("Proj::Far", &VulkanShadowMapping::g_LightFar, 0.1f, 100.0f);
	ImGui::SliderFloat("Pos::Dist", &VulkanShadowMapping::g_LightDist, 0.5f, 100.0f);
	ImGui::SliderFloat("Pos::AngleX", &VulkanShadowMapping::g_LightXAngle, -3.15f, +3.15f);
	ImGui::SliderFloat("Pos::AngleY", &VulkanShadowMapping::g_LightYAngle, -3.15f, +3.15f);
	ImGui::End();

	imguiTextureWindow("Color", 1);
	imguiTextureWindow("Depth", 2 | (0x1 << 16));
}

void ShadowMappingApp::draw3D()
{
	const mat4 proj = getDefaultProjection();
	const mat4 view = glm::scale(mat4(1.f), vec3(1.f, -1.f, 1.f)) * camera.getViewMatrix();

	const mat4 m1 = glm::rotate(
		glm::translate(mat4(1.0f), vec3(0.f, 0.5f, -1.5f)) * glm::rotate(mat4(1.f), -0.5f * glm::pi<float>(), vec3(1, 0, 0)),
		VulkanShadowMapping::g_ModelAngle, vec3(0.0f, 0.0f, 1.0f));

	canvas.setCameraMatrix(proj * view);

	quads.clear();
	quads.quad(-1.0f, -1.0f, 1.0f, 1.0f, 0);
	canvas.clear();

	const glm::mat4 rotY = glm::rotate(mat4(1.f), VulkanShadowMapping::g_LightYAngle, glm::vec3(0, 1, 0));
	const glm::mat4 rotX = glm::rotate(rotY, VulkanShadowMapping::g_LightXAngle, glm::vec3(1, 0, 0));
	const glm::vec4 lightPos = rotX * glm::vec4(0, 0, VulkanShadowMapping::g_LightDist, 1.0f);

	const mat4 lightProj = glm::perspective(glm::radians(VulkanShadowMapping::g_LightAngle), 1.0f, VulkanShadowMapping::g_LightNear, VulkanShadowMapping::g_LightFar);
	const mat4 lightView = glm::lookAt(glm::vec3(lightPos), vec3(0), vec3(0, 1, 0));

	renderCameraFrustum(canvas, lightView, lightProj, vec4(0.0f, 1.0f, 0.0f, 1.0f));

	Uniforms uniDepth{};
	uniDepth.mvp = lightProj * lightView * m1;
	uniDepth.model = m1;
	uniDepth.shadowMatrix = mat4(1.0f);
	uniDepth.cameraPos = vec4(camera.getPosition(), 1.0f);
	uniDepth.lightPos = lightPos;
	uniDepth.meshScale = VulkanShadowMapping::g_meshScale;
	uploadBufferData(ctx_.vkDev, shadowUniformBuffer.memory, 0, &uniDepth, sizeof(uniDepth));

	Uniforms uni{};
	uni.mvp = proj * view * m1;
	uni.model = m1;
	uni.shadowMatrix = lightProj * lightView;
	uni.cameraPos = vec4(camera.getPosition(), 1.0f);
	uni.lightPos = lightPos;
	uni.meshScale = VulkanShadowMapping::g_meshScale;
	uploadBufferData(ctx_.vkDev, meshUniformBuffer.memory, 0, &uni, sizeof(uni));
}
