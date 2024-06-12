#pragma once

#include <Mesh/BaseMesh.hpp>
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class BaseModel
{
public:
	BaseModel(std::string const& path, bool gamma = false);
	virtual ~BaseModel() {}

	virtual void Draw(ShaderInterface& shader);
	virtual BaseMesh* CreateMeshInstance(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
	virtual unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);

	const std::vector<BaseMesh*>& GetMeshes() const
	{
		return meshes;
	}
	const std::vector<Texture>& GetTexturesLoaded() const
	{
		return textures_loaded;
	}


protected:
	// model data
	std::vector<BaseMesh*> meshes;
	std::string directory{};
	std::vector<Texture> textures_loaded;
	bool gammaCorrection{ false };

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	BaseMesh* processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTexture(aiMaterial* material, aiTextureType type, std::string typeName);
};

