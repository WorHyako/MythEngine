#pragma once

#include "glm/glm.hpp"
#include "glm/matrix.hpp"

#include <Renderer/ShaderInterface.hpp>

#include <string>
#include <vector>

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
	// bone indices which will influence this vertex
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	// weights from each bone
	float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture
{
	unsigned int id;
	std::string type;
	std::string path; // we store the path of the texture to compare with other textures
};

class BaseMesh
{
public:
	// mesh data
	std::vector<Vertex>			vertices;
	std::vector<unsigned int>	indices;
	std::vector<Texture>		textures;

	BaseMesh();
	BaseMesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
	virtual ~BaseMesh();

	virtual void SetupMesh();
	virtual void Draw(ShaderInterface& shader);
private:
	// render data
};
