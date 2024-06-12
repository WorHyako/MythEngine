#pragma once

#include <Mesh/BaseModel.hpp>
#include <RHI/OpenGL/OpenGLBaseMesh.hpp>

class OpenGLBaseModel : public BaseModel
{
public:
	OpenGLBaseModel(std::string const& path, bool gamma = false);
	~OpenGLBaseModel() override = default;

	virtual void Draw(ShaderInterface& shader) override;
	virtual BaseMesh* CreateMeshInstance(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures) override;

	virtual unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma) override;
protected:

private:
	// model data
};

