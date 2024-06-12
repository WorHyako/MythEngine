#pragma once

#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include <RHI/OpenGL/Framework/GLShader.hpp>
#include <RHI/OpenGL/Framework/GLTexture.hpp>

class GLSkyboxRenderer
{
public:
	GLSkyboxRenderer(const char* envMap, const char* envMapIrradiance);
	~GLSkyboxRenderer();

	void draw();

private:
	// https://hdrihaven.com/hdri/?h=immenstadter_horn
	GLTexture envMap_;
	GLTexture envMapIrradiance_;
	GLTexture brdfLUT_;
	GLShader shdCubeVertex_;
	GLShader shdCubeFragment_;
	GLProgram progCube_;
	GLuint dummyVAO_;
};