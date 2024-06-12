#pragma once

class GLApp;
struct GLFWwindow;

class OpenGLBaseRender
{
public:
	OpenGLBaseRender(GLApp* app);

	virtual int draw() = 0;
	virtual void buildShaders(){};
	virtual void buildBuffers(){};

	GLApp* getApp() const;

protected:
	GLApp* app_;
	GLFWwindow* window_;
};