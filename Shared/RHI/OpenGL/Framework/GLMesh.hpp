#pragma once

#include <functional>
#include <RHI/OpenGL/Framework/GLFWApp.hpp>

#include <RHI/OpenGL/Framework/GLShader.hpp>

const GLuint kBufferIndex_PerFrameUniforms = 0;
const GLuint kBufferIndex_ModelMatrices = 1;
const GLuint kBufferIndex_Materials = 2;

class GLSceneData;

struct DrawElementsIndirectCommand
{
	GLuint count_;
	GLuint instanceCount_;
	GLuint firstIndex_;
	GLuint baseVertex_;
	GLuint baseInstance_;
};

class GLIndirectMesh final
{
public:
	explicit GLIndirectMesh(const GLSceneData& data);
	~GLIndirectMesh();

	void draw(const GLSceneData& data) const;

	GLIndirectMesh(const GLIndirectMesh&) = delete;
	GLIndirectMesh(GLIndirectMesh&&) = default;

private:
	GLuint vao_;
	uint32_t numIndices_;

	GLBuffer bufferVertices_;
	GLBuffer bufferIndices_;
	GLBuffer bufferMaterials_;

	GLBuffer bufferIndirect_;

	GLBuffer bufferModelMatrices_;
};

class GLIndirectBuffer final
{
public:
	explicit GLIndirectBuffer(size_t maxDrawCommands)
		: bufferIndirect_(sizeof(DrawElementsIndirectCommand) * maxDrawCommands, nullptr, GL_DYNAMIC_STORAGE_BIT)
		, drawCommands_(maxDrawCommands)
	{}

	GLuint getHandle() const { return bufferIndirect_.getHandle(); }
	void uploadIndirectBuffer();

	void selectTo(GLIndirectBuffer& buf, const std::function<bool(const DrawElementsIndirectCommand&)>& pred);

	std::vector<DrawElementsIndirectCommand> drawCommands_;
private:
	GLBuffer bufferIndirect_;
};

template<typename GLSceneDataType>
class GLMesh final
{
public:
	explicit GLMesh(const GLSceneDataType& data);
	~GLMesh();

	void updateMaterialsBuffer(const GLSceneDataType& data);

	void draw(size_t numDrawCommands, const GLIndirectBuffer* buffer = nullptr) const;

	GLMesh(const GLMesh&) = delete;
	GLMesh(GLMesh&&) = default;

	//private:
	GLuint vao_;
	uint32_t numIndices_;

	GLBuffer bufferIndices_;
	GLBuffer bufferVertices_;
	GLBuffer bufferMaterials_;
	GLBuffer bufferModelMatrices_;

	GLIndirectBuffer bufferIndirect_;
};