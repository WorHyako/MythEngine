#pragma once

#ifndef __gl_h_
#include <glad/gl.h>
#endif
#include "GLFW/glfw3.h"
#include <glm/vec2.hpp>
#include <functional>

class CameraPositioner_FirstPerson;

struct MouseState
{
	glm::vec2 pos = glm::vec2(0.0f);
	bool pressedLeft = false;
};

class GLFWUserInput
{
public:
	GLFWUserInput();

	static GLFWUserInput& GetInput()
	{
		static GLFWUserInput input;
		return input;
	}
	
	void processInput(GLFWwindow* window);
	void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
	void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void mousebutton_callback(GLFWwindow* window, int button, int action, int mods, bool ioWantCaptureMouse);

public:
	float deltaTime = 0.0f;
	float lastX = 0.0f;
	float lastY = 0.0f;
	bool bUseBlinnPhong = false;
	bool bIsFirstMouseEvent = true;
	bool freezeCullingView = false;
	CameraPositioner_FirstPerson* positioner;
	MouseState* mouseState = nullptr;

	std::function<void(double, double)> mousebutton_Callback;
};