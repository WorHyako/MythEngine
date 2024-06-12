#include <RHI/Vulkan/VulkanPhysicsRender.hpp>

PhysicsApp::PhysicsApp()
	: CameraApp(-90, -90)
	, plane(ctx_)
	, sceneData(ctx_,
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/cube.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/cube.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/cube.material").c_str(),
		{}, {})
	, multiRenderer(ctx_, sceneData,
		(FilesystemUtilities::GetShadersDir() + "Vulkan/SimpleCube/SimpleCube.vert").c_str(),
		(FilesystemUtilities::GetShadersDir() + "Vulkan/SimpleCube/SimpleCube.frag").c_str())
	, imgui(ctx_)
{
	positioner = CameraPositioner_FirstPerson(glm::vec3(0.0f, 50.0f, 100.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),

	onScreenRenderers_.emplace_back(plane, false);
	onScreenRenderers_.emplace_back(multiRenderer);
	onScreenRenderers_.emplace_back(imgui, false);

	const int maxCubes = 1000;

	for (int i = 0; i < maxCubes; i++)
		physics.addBox(vec3(1), btQuaternion(0, 0, 0, 1), vec3(0.f, 2.f + 3.f * i, 0.f), 3.f);
}


void PhysicsApp::draw3D()
{
	const mat4 p = getDefaultProjection();
	const mat4 view = camera.getViewMatrix();

	multiRenderer.setMatrices(p, view);
	multiRenderer.setCameraPosition(positioner.getPosition());

	plane.setMatrices(p, view, glm::mat4(1.f));

	sceneData.scene_.globalTransform_[0] = glm::mat4(1.f);
	for (size_t i = 0; i < physics.boxTransforms.size(); i++)
		sceneData.scene_.globalTransform_[i] = physics.boxTransforms[i];
}

void PhysicsApp::update(float deltaSeconds)
{
	CameraApp::update(deltaSeconds);

	physics.update(deltaSeconds);

	sceneData.uploadGlobalTransforms();
}

void generateMeshFile()
{
	constexpr uint32_t cubeVtxCount = 8;
	constexpr uint32_t cubeIdxCount = 36;

	Mesh cubeMesh{};
	cubeMesh.lodCount = 1;
	cubeMesh.streamCount = 1;
	cubeMesh.lodOffset[0] = 0;
	cubeMesh.lodOffset[1] = cubeIdxCount;
	cubeMesh.streamOffset[0] = 0;

	MeshData md{};
	md.indexData_ = std::vector<uint32_t>(cubeIdxCount, 0);
	md.vertexData_ = std::vector<float>(cubeVtxCount * 8, 0);
	md.meshes_ = std::vector<Mesh>{ cubeMesh };
	md.boxes_ = std::vector<BoundingBox>(1);

	saveMeshData((FilesystemUtilities::GetResourcesDir() + "Data/meshes/cube.meshes").c_str(), md);
}

void generateData()
{
	const int numCubes = 1000;

	Scene cubeScene;
	addNode(cubeScene, -1, 0);
	for(int i = 0; i < numCubes; i++)
	{
		addNode(cubeScene, 0, 1);
		cubeScene.meshes_[i + 1] = 0;
		cubeScene.materialForNode_[i + 1] = 0;
	}

	saveScene((FilesystemUtilities::GetResourcesDir() + "Data/meshes/cube.scene").c_str(), cubeScene);

	std::vector<std::string> files = { "textures/ch2_sample3_STB.jpg" };
	MaterialDescription md{};
	md.albedoColor_ = gpuvec4(1, 0, 0, 1);
	md.albedoMap_ = 0xFFFFFFFF;
	md.normalMap_ = 0xFFFFFFFF;
	std::vector<MaterialDescription> materials = { md };
	saveMaterials((FilesystemUtilities::GetResourcesDir() + "Data/meshes/cube.material").c_str(), materials, files);
}