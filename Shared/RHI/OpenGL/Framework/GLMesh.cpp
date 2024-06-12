#include <RHI/OpenGL/Framework/GLMesh.hpp>

#include <RHI/OpenGL/Framework/GLSceneData.hpp>
#include <RHI/OpenGL/Framework/GLSceneDataLazy.hpp>
#include <Scene/Mareial.hpp>

GLIndirectMesh::GLIndirectMesh(const GLSceneData& data)
	: numIndices_(data.header_.indexDataSize / sizeof(uint32_t))
	, bufferIndices_(data.header_.indexDataSize, data.meshData_.indexData_.data(), 0)
	, bufferVertices_(data.header_.vertexDataSize, data.meshData_.vertexData_.data(), 0)
	, bufferMaterials_(sizeof(MaterialDescription)* data.materials_.size(), data.materials_.data(), 0)
	, bufferIndirect_(sizeof(DrawElementsIndirectCommand)* data.shapes_.size() + sizeof(GLsizei), nullptr, GL_DYNAMIC_STORAGE_BIT)
	, bufferModelMatrices_(sizeof(glm::mat4)* data.shapes_.size(), nullptr, GL_DYNAMIC_STORAGE_BIT)
{
	glCreateVertexArrays(1, &vao_);
	glVertexArrayElementBuffer(vao_, bufferIndices_.getHandle());
	glVertexArrayVertexBuffer(vao_, 0, bufferVertices_.getHandle(), 0, sizeof(vec3) + sizeof(vec3) + sizeof(vec2));
	// position
	glEnableVertexArrayAttrib(vao_, 0);
	glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao_, 0, 0);
	// uv
	glEnableVertexArrayAttrib(vao_, 1);
	glVertexArrayAttribFormat(vao_, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vec3));
	glVertexArrayAttribBinding(vao_, 1, 0);
	// normal
	glEnableVertexArrayAttrib(vao_, 2);
	glVertexArrayAttribFormat(vao_, 2, 3, GL_FLOAT, GL_TRUE, sizeof(vec3) + sizeof(vec2));
	glVertexArrayAttribBinding(vao_, 2, 0);

	std::vector<uint8_t> drawCommands;

	drawCommands.resize(sizeof(DrawElementsIndirectCommand) * data.shapes_.size() + sizeof(GLsizei));

	// store the number of draw commands in the very beginning of the buffer
	const GLsizei numCommands = (GLsizei)data.shapes_.size();
	memcpy(drawCommands.data(), &numCommands, sizeof(numCommands));

	DrawElementsIndirectCommand* cmd = std::launder(
		reinterpret_cast<DrawElementsIndirectCommand*>(drawCommands.data() + sizeof(GLsizei))
	);

	// prepare indirect commands buffer
	for (size_t i = 0; i != data.shapes_.size(); i++)
	{
		const uint32_t meshIdx = data.shapes_[i].meshIndex;
		const uint32_t lod = data.shapes_[i].LOD;

		DrawElementsIndirectCommand indirectCommand{};
		indirectCommand.count_ = data.meshData_.meshes_[meshIdx].getLODIndicesCount(lod);
		indirectCommand.instanceCount_ = 1;
		indirectCommand.firstIndex_ = data.shapes_[i].indexOffset;
		indirectCommand.baseVertex_ = data.shapes_[i].vertexOffset;
		indirectCommand.baseInstance_ = data.shapes_[i].materialIndex;

		*cmd++ = indirectCommand;
	}

	glNamedBufferSubData(bufferIndirect_.getHandle(), 0, drawCommands.size(), drawCommands.data());

	std::vector<glm::mat4> matrices(data.shapes_.size());
	size_t i = 0;
	for (const auto& c : data.shapes_)
		matrices[i++] = data.scene_.globalTransform_[c.transformIndex];

	glNamedBufferSubData(bufferModelMatrices_.getHandle(), 0, matrices.size() * sizeof(mat4), matrices.data());
}

void GLIndirectMesh::draw(const GLSceneData& data) const
{
	glBindVertexArray(vao_);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_Materials, bufferMaterials_.getHandle());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_ModelMatrices, bufferModelMatrices_.getHandle());
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, bufferIndirect_.getHandle());
	glBindBuffer(GL_PARAMETER_BUFFER, bufferIndirect_.getHandle());
	glMultiDrawElementsIndirectCount(GL_TRIANGLES, GL_UNSIGNED_INT, (const void*)sizeof(GLsizei), 0, (GLsizei)data.shapes_.size(), 0);
}

GLIndirectMesh::~GLIndirectMesh()
{
	glDeleteVertexArrays(1, &vao_);
}

void GLIndirectBuffer::uploadIndirectBuffer()
{
	glNamedBufferSubData(bufferIndirect_.getHandle(), 0, sizeof(DrawElementsIndirectCommand) * drawCommands_.size(), drawCommands_.data());
}

void GLIndirectBuffer::selectTo(GLIndirectBuffer& buf, const std::function<bool(const DrawElementsIndirectCommand&)>& pred)
{
	buf.drawCommands_.clear();
	for(const auto& c : drawCommands_)
	{
		if (pred(c))
			buf.drawCommands_.push_back(c);
	}
	buf.uploadIndirectBuffer();
}

template class GLMesh<GLSceneData>;
template class GLMesh<GLSceneDataLazy>;

template <class GLSceneDataType>
GLMesh<GLSceneDataType>::GLMesh(const GLSceneDataType& data)
	: numIndices_(data.header_.indexDataSize / sizeof(uint32_t))
	, bufferIndices_(data.header_.indexDataSize, data.meshData_.indexData_.data(), 0)
	, bufferVertices_(data.header_.vertexDataSize, data.meshData_.vertexData_.data(), 0)
	, bufferMaterials_(sizeof(MaterialDescription)* data.materials_.size(), data.materials_.data(), GL_DYNAMIC_STORAGE_BIT)
	, bufferModelMatrices_(sizeof(glm::mat4)* data.shapes_.size(), nullptr, GL_DYNAMIC_STORAGE_BIT)
	, bufferIndirect_(data.shapes_.size())
{
	glCreateVertexArrays(1, &vao_);
	glVertexArrayElementBuffer(vao_, bufferIndices_.getHandle());
	glVertexArrayVertexBuffer(vao_, 0, bufferVertices_.getHandle(), 0, sizeof(vec3) + sizeof(vec3) + sizeof(vec2));
	// position
	glEnableVertexArrayAttrib(vao_, 0);
	glVertexArrayAttribFormat(vao_, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao_, 0, 0);
	// uv
	glEnableVertexArrayAttrib(vao_, 1);
	glVertexArrayAttribFormat(vao_, 1, 2, GL_FLOAT, GL_FALSE, sizeof(vec3));
	glVertexArrayAttribBinding(vao_, 1, 0);
	// normal
	glEnableVertexArrayAttrib(vao_, 2);
	glVertexArrayAttribFormat(vao_, 2, 3, GL_FLOAT, GL_TRUE, sizeof(vec3) + sizeof(vec2));
	glVertexArrayAttribBinding(vao_, 2, 0);

	std::vector<glm::mat4> matrices(data.shapes_.size());

	// prepare indirect commands buffer
	for (size_t i = 0; i != data.shapes_.size(); i++)
	{
		const uint32_t meshIdx = data.shapes_[i].meshIndex;
		const uint32_t lod = data.shapes_[i].LOD;
		bufferIndirect_.drawCommands_[i].count_ = data.meshData_.meshes_[meshIdx].getLODIndicesCount(lod);
		bufferIndirect_.drawCommands_[i].instanceCount_ = 1;
		bufferIndirect_.drawCommands_[i].firstIndex_ = data.shapes_[i].indexOffset;
		bufferIndirect_.drawCommands_[i].baseVertex_ = data.shapes_[i].vertexOffset;
		bufferIndirect_.drawCommands_[i].baseInstance_ = data.shapes_[i].materialIndex + (uint32_t(i) << 16);
		matrices[i] = data.scene_.globalTransform_[data.shapes_[i].transformIndex];
	}

	bufferIndirect_.uploadIndirectBuffer();

	glNamedBufferSubData(bufferModelMatrices_.getHandle(), 0, matrices.size() * sizeof(mat4), matrices.data());
}

template <class GLSceneDataType>
void GLMesh<GLSceneDataType>::updateMaterialsBuffer(const GLSceneDataType& data)
{
	glNamedBufferSubData(bufferMaterials_.getHandle(), 0, sizeof(MaterialDescription) * data.materials_.size(), data.materials_.data());
}

template <class GLSceneDataType>
void GLMesh<GLSceneDataType>::draw(size_t numDrawCommands, const GLIndirectBuffer* buffer) const
{
	glBindVertexArray(vao_);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_Materials, bufferMaterials_.getHandle());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_ModelMatrices, bufferModelMatrices_.getHandle());
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, (buffer ? *buffer : bufferIndirect_).getHandle());
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, (GLsizei)numDrawCommands, 0);
}

template <class GLSceneDataType>
GLMesh<GLSceneDataType>::~GLMesh()
{
	glDeleteVertexArrays(1, &vao_);
}
