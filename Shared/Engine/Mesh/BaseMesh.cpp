#include <Mesh/BaseMesh.hpp>
#include <glad/gl.h> // holds all OpenGL type declarations

BaseMesh::BaseMesh()
{
	
}

BaseMesh::BaseMesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	SetupMesh();
}

BaseMesh::~BaseMesh()
{
	
}

void BaseMesh::SetupMesh()
{
	
}

void BaseMesh::Draw(ShaderInterface& shader)
{
	
}