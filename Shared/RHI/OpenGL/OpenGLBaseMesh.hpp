#pragma once

#include <Mesh/BaseMesh.hpp>

class OpenGLBaseMesh : public BaseMesh
{
public:
	OpenGLBaseMesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
	virtual ~OpenGLBaseMesh() {};

	virtual void Draw(ShaderInterface& shader) override;
	virtual void SetupMesh() override;

public:
	// mesh data
	unsigned int VAO;
private:
	// render data
	unsigned int VBO, EBO;
};
