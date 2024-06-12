#include <UserInput/GLFW/GLFWUserInput.hpp>

#include "Camera/BaseCamera.hpp"
#include "Camera/TestCamera.hpp"

GLFWUserInput::GLFWUserInput()
	: positioner(new CameraPositioner_FirstPerson(glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f)))
{
	mouseState = new MouseState();
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void GLFWUserInput::processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	const float cameraSpeed = static_cast<float>(2.5f * deltaTime);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		Camera::GetCamera().ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		Camera::GetCamera().ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		Camera::GetCamera().ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		Camera::GetCamera().ProcessKeyboard(RIGHT, deltaTime);

	bUseBlinnPhong = glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void GLFWUserInput::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw:: whenever the mouse moves, this callback is called
// --------------------------------------------------------
void GLFWUserInput::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (bIsFirstMouseEvent)
	{
		lastX = xpos;
		lastY = ypos;
		bIsFirstMouseEvent = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	Camera::GetCamera().ProcessMouseMovement(xoffset, yoffset);

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	mouseState->pos.x = static_cast<float>(xposIn / width);
	mouseState->pos.y = static_cast<float>(yposIn / height);
}

// glfw:: whenever the mouse scroll wheel scrolls, this callback is called
// -----------------------------------------------------------------------
void GLFWUserInput::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	Camera::GetCamera().ProcessMouseScroll(static_cast<float>(yoffset));
}

void GLFWUserInput::keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	const bool pressed = action != GLFW_RELEASE;
	if (key == GLFW_KEY_ESCAPE && pressed)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_W)
		positioner->movement_.forward_ = pressed;
	if (key == GLFW_KEY_S)
		positioner->movement_.backward_ = pressed;
	if (key == GLFW_KEY_A)
		positioner->movement_.left_ = pressed;
	if (key == GLFW_KEY_D)
		positioner->movement_.right_ = pressed;
	if (key == GLFW_KEY_E)
		positioner->movement_.up_ = pressed;
	if (key == GLFW_KEY_Q)
		positioner->movement_.down_ = pressed;
	if (mods & GLFW_MOD_SHIFT)
		positioner->movement_.fastSpeed_ = pressed;
	if (key == GLFW_KEY_SPACE)
		positioner->setUpVector(glm::vec3(0.0f, 1.0f, 0.0f));

	if (key == GLFW_KEY_P && pressed)
		freezeCullingView = !freezeCullingView;
}

void GLFWUserInput::mousebutton_callback(GLFWwindow* window, int button, int action, int mods, bool ioWantCaptureMouse)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && !ioWantCaptureMouse)
		mouseState->pressedLeft = action == GLFW_PRESS;
    if (mousebutton_Callback)
        mousebutton_Callback(lastX, lastY);
}