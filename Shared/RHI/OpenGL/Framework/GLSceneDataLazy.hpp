#pragma once

#include <mutex>

#include <Scene/Scene.hpp>
#include <Scene/Mareial.hpp>
#include <Scene/VtxData.hpp>
#include <RHI/OpenGL/Framework/GLTexture.hpp>
#include <taskflow/taskflow.hpp>

#include <Filesystem/FilesystemUtilities.hpp>

class GLSceneDataLazy
{
public:
	GLSceneDataLazy(
		const char* meshFile,
		const char* sceneFile,
		const char* materialFile);

	struct LoadedImageData
	{
		int index_ = 0;
		int w_ = 0;
		int h_ = 0;
		const uint8_t* img_ = nullptr;
	};

	const std::shared_ptr<GLTexture> dummyTexture_ = std::make_shared<GLTexture>(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "textures/const1.bmp").c_str() );

	std::vector<std::string> textureFiles_;
	std::vector<LoadedImageData> loadedFiles_;
	std::mutex loadedFilesMutex_;
	std::vector<std::shared_ptr<GLTexture>> allMaterialTextures_;

	MeshFileHeader header_;
	MeshData meshData_;

	Scene scene_;
	std::vector<MaterialDescription> materialsLoaded_; // materials loaded from scene
	std::vector<MaterialDescription> materials_; // materials uploaded to GPU buffers
	std::vector<DrawData> shapes_;

	tf::Taskflow taskflow_;
	tf::Executor executor_;

	bool uploadLoadedTextures();

private:
	void loadScene(const char* sceneFile);
	void updateMaterials();
};
