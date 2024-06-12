#include <RHI/OpenGL/Framework/GLSkyboxRenderer.hpp>

#include <Filesystem/FilesystemUtilities.hpp>

GLSkyboxRenderer::GLSkyboxRenderer(const char* envMap, const char* envMapIrradiance)
	: envMap_(GL_TEXTURE_CUBE_MAP, envMap)
	, envMapIrradiance_(GL_TEXTURE_CUBE_MAP, envMapIrradiance)
	, brdfLUT_(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "Data/brdfLUT.ktx").c_str())
	, shdCubeVertex_((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/Cube.vert").c_str())
	, shdCubeFragment_((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/Cube.frag").c_str())
	, progCube_(shdCubeVertex_, shdCubeFragment_)
{
	glCreateVertexArrays(1, &dummyVAO_);
	const GLuint pbrTextures[] = { envMap_.getHandle(), envMapIrradiance_.getHandle(), brdfLUT_.getHandle() };
	// binding points for data/shaders/PBR.sp
	glBindTextures(5, 3, pbrTextures);
}


GLSkyboxRenderer::~GLSkyboxRenderer()
{
	glDeleteVertexArrays(1, &dummyVAO_);
}

void GLSkyboxRenderer::draw()
{
	progCube_.useProgram();
	glBindTextureUnit(1, envMap_.getHandle());
	glDepthMask(false);
	glBindVertexArray(dummyVAO_);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(true);
}
