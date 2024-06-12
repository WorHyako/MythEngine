#include <RHI/Vulkan/VulkanLargeSceneRender.hpp>

#include "Filesystem/FilesystemUtilities.hpp"

LargeSceneApp::LargeSceneApp()
	: CameraApp(-95, -95)
	, envMap(ctx_.resources.loadCubemap((FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k.hdr").c_str()))
	, irrMap(ctx_.resources.loadCubemap((FilesystemUtilities::GetResourcesDir() + "textures/piazza_bologni_1k_irradiance.hdr").c_str()))
	, sceneData(ctx_, 
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.materials").c_str(),
		envMap, irrMap)
	, sceneData2(ctx_,
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test2.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test2.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test2.materials").c_str(),
		envMap, irrMap)
	, muiltiRenderer(ctx_, sceneData)
	, multiRenderer2(ctx_, sceneData2)
	, imgui(ctx_)
{
	positioner = CameraPositioner_FirstPerson(glm::vec3(-10.0f, -3.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f));

	onScreenRenderers_.emplace_back(muiltiRenderer);
	onScreenRenderers_.emplace_back(multiRenderer2);
	onScreenRenderers_.emplace_back(imgui, false);
}

void LargeSceneApp::draw3D()
{
	const mat4 p = getDefaultProjection();
	const mat4 view = camera.getViewMatrix();

	muiltiRenderer.setMatrices(p, view);
	multiRenderer2.setMatrices(p, view);

	muiltiRenderer.setCameraPosition(positioner.getPosition());
	multiRenderer2.setCameraPosition(positioner.getPosition());
}

