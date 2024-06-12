#include <RHI/OpenGL/OpenGLBaseRender.hpp>
#include <RHI/OpenGL/Framework/GLFWApp.hpp>

OpenGLBaseRender::OpenGLBaseRender(GLApp* app)
	: app_(app)
	, window_(app->getWindow())
{}

GLApp* OpenGLBaseRender::getApp() const
{
	return app_;
}