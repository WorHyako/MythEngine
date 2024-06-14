#include <RHI/OpenGL/OpenGLWindow.hpp>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <Filesystem/FilesystemUtilities.hpp>
#include <RHI/OpenGL/OpenGLRender.hpp>
#include <RHI/OpenGL/OpenGLStudyRender.hpp>
#include <RHI/OpenGL/OpenGL_GLTF_PBRRender.hpp>
#include <RHI/OpenGL/OpenGLLargeSceneRender.hpp>
#include <RHI/OpenGL/OpenGLShadowMapping.hpp>
#include <RHI/OpenGL/OpenGLSSAORender.hpp>
#include <RHI/OpenGL/OpenGLHDRRender.hpp>
#include <RHI/OpenGL/OpenGLCullingCPURender.hpp>
#include <RHI/OpenGL/OpenGLCullingGPURender.hpp>
#include <RHI/OpenGL/OpenGLOITransparency.hpp>
#include <RHI/OpenGL/OpenGLSceneCompositionRender.hpp>
#include <RHI/OpenGL/OpenGLSceneRenderer.hpp>
#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include <UserInput/GLFW/GLFWUserInput.hpp>

OpenGLWindow::OpenGLWindow()
	:WindowInterface()
{
	app = new GLApp(&WindowParams);
	window = app->getWindow();

	render = new OpenGLSceneCompositionRender(app);
	render->buildBuffers();
	render->buildShaders();

	GLFWUserInput* userInput = &GLFWUserInput::GetInput();
	glfwSetWindowUserPointer(window, userInput);

	InitializeCallbacks();
	// glfw: initialize and configure
	// ------------------------------
//	glfwSetErrorCallback(
//		[](int errorCode, const char* description)
//		{
//			// TODO error callback
//		});
//
//	if(!glfwInit())
//	{
//		exit(EXIT_FAILURE);
//	}
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
//	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
//	glfwWindowHint(GLFW_SAMPLES, 4);
//
//#ifdef __APPLE__
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//#endif
//
//	window = glfwCreateWindow(WindowParams.Width, WindowParams.Height, WindowParams.WindowTitle.c_str(), NULL, NULL);
//	if (window)
//	{
//		glfwMakeContextCurrent(window);
//
//		GLFWUserInput* userInput = &GLFWUserInput::GetInput();
//
//		glfwSetWindowUserPointer(window, userInput);
//
//		InitializeCallbacks();
//
//		// tell GLFW to capture our mouse
//		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
//
//		if (!gladLoadGL(glfwGetProcAddress))
//		{
//			// TODO Log
//			//std::cout << "Failed to initialize GLAD" << std::endl;
//			//return -1;
//		}
//
//		glfwSwapInterval(1);
//	}
//	else
//	{
//		// TODO Log
//		//std::cout << "Failed to create GLFW window" << std::endl;
//		glfwTerminate();
//	}
}

OpenGLWindow::~OpenGLWindow()
{
	
}

int OpenGLWindow::Run()
{
	//while (!glfwWindowShouldClose(window))
	//{
	//	// Start the Dear ImGui Frame
	//	ImGui_ImplOpenGL3_NewFrame();
	//	ImGui_ImplGlfw_NewFrame();
	//	ImGui::NewFrame();

	//	ImGui::Begin("start window");
	//	ImGui::SetWindowSize(ImVec2(300.0f, 100.0f));
	//	ImGui::Text("This is some useful text.");

	//	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	//	ImGui::End();

	//	ImGui::Render();

	//	if(render)
	//	{
	//		render->Draw(window);
	//	}

	//	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	//	glfwSwapBuffers(window);
	//	glfwPollEvents();
	//}
	//glfwTerminate();

	if (render)
	{
		render->draw();
	}

	// ImGui: terminate, clearing all previously allocated ImGui resources.
	// --------------------------------------------------------------------
	if(ImGui::GetCurrentContext())
	{
		ImGui::DestroyContext();
	}

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwDestroyWindow(window);
	glfwTerminate();

	return EXIT_SUCCESS;
}

void OpenGLWindow::InitializeCallbacks()
{
	auto keyboard_callback_func = [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->keyboard_callback(window, key, scancode, action, mods);
	};
	glfwSetKeyCallback(window, keyboard_callback_func);

	auto framebuffer_size_callback_func = [](GLFWwindow* window, int width, int height)
	{
		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->framebuffer_size_callback(window, width, height);
	};
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback_func);

	auto mouse_callback_func = [](GLFWwindow* window, double xposIn, double yposIn)
	{
		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->mouse_callback(window, xposIn, yposIn);

		if(ImGui::GetCurrentContext())
		{
			auto& io = ImGui::GetIO();
			io.MousePos = ImVec2(xposIn, yposIn);
		}
	};
	glfwSetCursorPosCallback(window, mouse_callback_func);

	auto scroll_callback_func = [](GLFWwindow* window, double xoffset, double yoffset)
	{
		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->scroll_callback(window, xoffset, yoffset);
	};
	glfwSetScrollCallback(window, scroll_callback_func);

	glfwSetMouseButtonCallback(window, [](auto* window, int button, int action, int mods)
	{
		int idx = button == GLFW_MOUSE_BUTTON_LEFT ? 0 : button == GLFW_MOUSE_BUTTON_RIGHT ? 2 : 1;
		bool ioWantCaptureMouse = false;

		if(ImGui::GetCurrentContext())
		{
			auto& io = ImGui::GetIO();
			io.MouseDown[idx] = action == GLFW_PRESS;
			ioWantCaptureMouse = io.WantCaptureMouse;
		}

		static_cast<GLFWUserInput*>(glfwGetWindowUserPointer(window))->mousebutton_callback(window, button, action, mods, ioWantCaptureMouse);
	});
}