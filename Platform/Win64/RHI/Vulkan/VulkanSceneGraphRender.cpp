#include <RHI/Vulkan/VulkanSceneGraphRender.hpp>

#include <ImGuizmo.h>
#ifndef _MATH_DEFINES_DEFINED
#define _MATH_DEFINES_DEFINED
// Definitions of useful mathematical constants
//
// Define _USE_MATH_DEFINES before including <math.h> to expose these macro
// definitions for common math constants.  These are placed under an #ifdef
// since these commonly-defined names are not part of the C or C++ standards
#define M_E        2.71828182845904523536   // e
#define M_LOG2E    1.44269504088896340736   // log2(e)
#define M_LOG10E   0.434294481903251827651  // log10(e)
#define M_LN2      0.693147180559945309417  // ln(2)
#define M_LN10     2.30258509299404568402   // ln(10)
#define M_PI       3.14159265358979323846   // pi
#define M_PI_2     1.57079632679489661923   // pi/2
#define M_PI_4     0.785398163397448309616  // pi/4
#define M_1_PI     0.318309886183790671538  // 1/pi
#define M_2_PI     0.636619772367581343076  // 2/pi
#define M_2_SQRTPI 1.12837916709551257390   // 2/sqrt(pi)
#define M_SQRT2    1.41421356237309504880   // sqrt(2)
#define M_SQRT1_2  0.707106781186547524401  // 1/sqrt(2)
#endif

SceneGraphApp::SceneGraphApp()
	: CameraApp(-95, -95)
	, envMap(ctx_.resources.loadCubemap((FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k.hdr").c_str()))
	, irrMap(ctx_.resources.loadCubemap((FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k_irradiance.hdr").c_str()))
	, sceneData(ctx_,
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test_graph.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test_graph.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test_graph.materials").c_str(),
		envMap, irrMap)
	, plane(ctx_)
	, muiltiRenderer(ctx_, sceneData)
	, imgui(ctx_)
{
	onScreenRenderers_.emplace_back(plane, false);
	onScreenRenderers_.emplace_back(muiltiRenderer);
	onScreenRenderers_.emplace_back(imgui, false);

	sceneData.scene_.localTransform_[0] = glm::rotate(glm::mat4(1.f), (float)(M_PI / 2.f), glm::vec3(1.f, 0.f, 0.0f));
}

void SceneGraphApp::drawUI()
{
	ImGui::Begin("Information", nullptr);
		ImGui::Text("FPS: %.2f", getFPS());
	ImGui::End();

	ImGui::Begin("Scene graph", nullptr);
		int node = renderSceneTree(sceneData.scene_, 0);
		if (node > -1)
			selectedNode = node;
	ImGui::End();

	editNode(selectedNode);
}

void SceneGraphApp::draw3D()
{
	const mat4 p = getDefaultProjection();
	const mat4 view = camera.getViewMatrix();

	muiltiRenderer.setMatrices(p, view);
	plane.setMatrices(p, view, mat4(1.0f));
}

void SceneGraphApp::update(float deltaSeconds)
{
	CameraApp::update(deltaSeconds);

	// update/upload matrices for individual scene nodes
	sceneData.recalculateAllTransforms();
	sceneData.uploadGlobalTransforms();
}

void SceneGraphApp::editNode(int node)
{
	if(node < 0)
		return;

	mat4 cameraProjection = getDefaultProjection();
	mat4 cameraView = camera.getViewMatrix();

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();

	std::string name = getNodeName(sceneData.scene_, node);
	std::string label = name.empty() ? (std::string("Node") + std::to_string(node)) : name;
	label = "Node: " + label;

	ImGui::Begin("Editor");
		ImGui::Text("%s", label.c_str());

		glm::mat4 globalTransform = sceneData.scene_.globalTransform_[node]; // fetch global transform
		glm::mat4 srcTransform = globalTransform;
		glm::mat4 localTransform = sceneData.scene_.localTransform_[node];

		ImGui::Separator();
		ImGuizmo::SetID(1);

		editTransform(cameraView, cameraProjection, globalTransform); // calculate delta for edited global transform
		glm::mat4 deltaTransform = glm::inverse(srcTransform) * globalTransform;
		sceneData.scene_.localTransform_[node] = localTransform * deltaTransform; // modify local transform
		markAsChanged(sceneData.scene_, node);

		ImGui::Separator();
		ImGui::Text("%s", "Material");

		editMaterial(node);
	ImGui::End();
}

void SceneGraphApp::editTransform(const glm::mat4& view, const glm::mat4& projection, glm::mat4& matrix)
{
	static ImGuizmo::OPERATION gizmoOperation(ImGuizmo::TRANSLATE);

	if (ImGui::RadioButton("Translate", gizmoOperation == ImGuizmo::TRANSLATE))
		gizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", gizmoOperation == ImGuizmo::ROTATE))
		gizmoOperation = ImGuizmo::ROTATE;

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmoOperation, ImGuizmo::WORLD, glm::value_ptr(matrix));
}

void SceneGraphApp::editMaterial(int node)
{
	if(!(sceneData.scene_.materialForNode_.count(node) > 0))
		return;

	auto matIdx = sceneData.scene_.materialForNode_.at(node);
	MaterialDescription& material = sceneData.materials_[matIdx];

	float emissiveColor[4];
	memcpy(emissiveColor, &material.emissiveColor_, sizeof(gpuvec4));
	gpuvec4 oldColor = material.emissiveColor_;
	ImGui::ColorEdit3("Emissive color", emissiveColor);

	if(memcmp(emissiveColor, &oldColor, sizeof(gpuvec4)))
	{
		memcpy(&material.emissiveColor_, emissiveColor, sizeof(gpuvec4));
		sceneData.updateMaterial(matIdx);
	}
}
