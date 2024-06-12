#include <RHI/OpenGL/Framework/LineCanvasGL.hpp>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

GLMeshPVP::GLMeshPVP(const std::vector<uint32_t>& indices, const void* vertexData, uint32_t verticesSize)
	: numIndices_((uint32_t)indices.size())
	, bufferIndices_(indices.size() ? std::make_unique<GLBuffer>(indices.size() * sizeof(uint32_t), indices.data(), 0) : nullptr)
	, bufferVertices_(std::make_unique<GLBuffer>(verticesSize, vertexData, GL_DYNAMIC_STORAGE_BIT))
{
	glCreateVertexArrays(1, &vao_);
	if (bufferIndices_)
		glVertexArrayElementBuffer(vao_, bufferIndices_->getHandle());
}

GLMeshPVP::GLMeshPVP(const char* fileName)
{
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	{
		const aiScene* scene = aiImportFile(fileName, aiProcess_Triangulate);
		if (!scene || !scene->HasMeshes())
		{
			printf("Unable to load '%s'\n", fileName);
			exit(255);
		}

		const aiMesh* mesh = scene->mMeshes[0];
		for (unsigned i = 0; i != mesh->mNumVertices; i++)
		{
			const aiVector3D v = mesh->mVertices[i];
			const aiVector3D n = mesh->mNormals[i];
			const aiVector3D t = mesh->mTextureCoords[0][i];
			vertices.push_back({ vec3(v.x, v.y, v.z), vec3(n.x, n.y, n.z), vec2(t.x, 1.0f - t.y) });
		}
		for (unsigned i = 0; i != mesh->mNumFaces; i++)
			for (unsigned j = 0; j != 3; j++)
				indices.push_back(mesh->mFaces[i].mIndices[j]);
		aiReleaseImport(scene);
	}
	const size_t kSizeIndices = sizeof(uint32_t) * indices.size();
	const size_t kSizeVertices = sizeof(VertexData) * vertices.size();
	numIndices_ = (uint32_t)indices.size();
	bufferIndices_ = std::make_unique<GLBuffer>((uint32_t)kSizeIndices, indices.data(), 0);
	bufferVertices_ = std::make_unique<GLBuffer>((uint32_t)kSizeVertices, vertices.data(), 0);
	glCreateVertexArrays(1, &vao_);
	glVertexArrayElementBuffer(vao_, bufferIndices_->getHandle());
}

void GLMeshPVP::drawElements(GLenum mode) const
{
	glBindVertexArray(vao_);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bufferVertices_->getHandle());
	glDrawElements(mode, static_cast<GLsizei>(numIndices_), GL_UNSIGNED_INT, nullptr);
}

void GLMeshPVP::drawArrays(GLenum mode, GLint first, GLint count)
{
	glBindVertexArray(vao_);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, bufferVertices_->getHandle());
	glDrawArrays(mode, first, count);
}

GLMeshPVP::~GLMeshPVP()
{
	glDeleteVertexArrays(1, &vao_);
}

void CanvasGL::line(const vec3& p1, const vec3& p2, const vec4& c)
{
	lines_.push_back({ p1, c });
	lines_.push_back({ p2, c });
}

void CanvasGL::flush()
{
	if (lines_.empty())
		return;

	assert(lines_.size() < kMaxLines);

	glNamedBufferSubData(mesh_.bufferVertices_->getHandle(), 0, uint32_t(lines_.size() * sizeof(VertexData)), lines_.data());
	progLines_.useProgram();
	mesh_.drawArrays(GL_LINES, 0, (GLint)lines_.size());

	lines_.clear();
}

//void renderCameraFrustumGL(CanvasGL& canvas, const mat4& camView, const mat4& camProj, const vec4& color, int numSegments)
