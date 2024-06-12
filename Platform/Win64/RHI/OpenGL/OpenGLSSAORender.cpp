#include <RHI/OpenGL/OpenGLSSAORender.hpp>

#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include <RHI/OpenGL/Framework/GLShader.hpp>
#include <RHI/OpenGL/Framework/GLSceneData.hpp>
#include <RHI/OpenGL/Framework/GLFramebuffer.hpp>
#include <RHI/OpenGL/Framework/UtilsGLImGui.hpp>
#include <RHI/OpenGL/OpenGLLargeSceneRender.hpp>
#include <RHI/OpenGL/Framework/GLMesh.hpp>

#include <Camera/TestCamera.hpp>
#include <UserInput/GLFW/GLFWUserInput.hpp>

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

struct SSAOParams
{
	float scale_ = 1.0f;
	float bias_ = 0.2f;
	float zNear = 0.1f;
	float zFar = 1000.0f;
	float radius = 0.2f;
	float attScale = 1.0f;
	float distScale = 0.5f;
} g_SSAOParams;

static_assert(sizeof(SSAOParams) <= sizeof(PerFrameData));

bool g_EnableSSAO = true;
bool g_EnableBlur = true;

OpenGLSSAORender::OpenGLSSAORender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(0.0f, 6.0f, 11.0f), vec3(0.0f, 4.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{
	
}

int OpenGLSSAORender::draw()
{
	GLFWUserInput& input = GLFWUserInput::GetInput();
	input.positioner = positioner_;
	positioner_->maxSpeed_ = 1.0f;

	GLShader shdGridVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.vert").c_str());
	GLShader shdGridFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.frag").c_str());
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	GLShader shaderVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/LargeSceneShader/LargeSceneShader.vert").c_str());
	GLShader shaderFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/LargeSceneShader/LargeSceneShader.frag").c_str());
	GLProgram program(shaderVertex, shaderFragment);

	GLShader shdFullScreenQuadVert((FilesystemUtilities::GetShadersDir() + "OpenGL/FullScreenQuad/FullScreenQuad.vert").c_str());

	GLShader shdSSAOFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/SSAO.frag").c_str());
	GLShader shdCombineSSAOFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/SSAO_Combine.frag").c_str());
	GLProgram progSSAO(shdFullScreenQuadVert, shdSSAOFrag);
	GLProgram progCombineSSAO(shdFullScreenQuadVert, shdCombineSSAOFrag);

	GLShader shdBlurXFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/BlurX.frag").c_str());
	GLShader shdBlurYFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/BlurY.frag").c_str());
	GLProgram progBlurX(shdFullScreenQuadVert, shdBlurXFrag);
	GLProgram progBlurY(shdFullScreenQuadVert, shdBlurYFrag);

	const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLBuffer perFrameDataBuffer(kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle(), 0, kUniformBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	GLTexture rotationTexture(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "textures/rot_texture.bmp").c_str());

	GLSceneData sceneData1(
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test.materials").c_str()
	);
	GLSceneData sceneData2(
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test2.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test2.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/test2.materials").c_str()
	);

	GLIndirectMesh mesh1(sceneData1);
	GLIndirectMesh mesh2(sceneData2);

	double timeStamp = glfwGetTime();
	float deltaSeconds = 0.0f;

	// offscreen render targets
	int width, height;
	glfwGetFramebufferSize(app_->getWindow(), &width, &height);
	GLFramebuffer framebuffer(width, height, GL_RGBA8, GL_DEPTH_COMPONENT24);
	GLFramebuffer ssao(1024, 1024, GL_RGBA8, 0);
	GLFramebuffer blur(1024, 1024, GL_RGBA8, 0);

	ImGuiGLRenderer rendererUI;

	while (!glfwWindowShouldClose(app_->getWindow()))
	{
		positioner_->update(deltaSeconds, input.mouseState->pos, input.mouseState->pressedLeft);

		const double newTimeStamp = glfwGetTime();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize(app_->getWindow(), &width, &height);
		const float ratio = width / (float)height;

		glClearNamedFramebufferfv(framebuffer.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glClearNamedFramebufferfi(framebuffer.getHandle(), GL_DEPTH_STENCIL, 0, 1.0f, 0);

		const mat4 p = glm::perspective(45.0f, ratio, g_SSAOParams.zNear, g_SSAOParams.zFar);
		const mat4 view = testCamera_->getViewMatrix();

		const PerFrameData perFrameData{ view, p, glm::vec4(testCamera_->getPosition(), 1.0f) };
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		// 1. Render scene
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		framebuffer.bind();
		// 1.1 Bistro
		program.useProgram();
		mesh1.draw(sceneData1);
		mesh2.draw(sceneData2);
		// 1.2 Grid
		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
		framebuffer.unbind();
		glDisable(GL_DEPTH_TEST);

		// 2. Calculate SSAO
		glClearNamedFramebufferfv(ssao.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, sizeof(g_SSAOParams), &g_SSAOParams);
		ssao.bind();
		progSSAO.useProgram();
		glBindTextureUnit(0, framebuffer.getTextureDepth().getHandle());
		glBindTextureUnit(1, rotationTexture.getHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);
		ssao.unbind();

		// 2.1 Blur SSAO
		if (g_EnableBlur)
		{
			// Blur X
			blur.bind();
			progBlurX.useProgram();
			glBindTextureUnit(0, ssao.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
			blur.unbind();
			// Blur Y
			ssao.bind();
			progBlurY.useProgram();
			glBindTextureUnit(0, blur.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
			ssao.unbind();
		}

		// 3. Combine SSAO and the rendered scene
		glViewport(0, 0, width, height);
		if(g_EnableSSAO)
		{
			progCombineSSAO.useProgram();
			glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
			glBindTextureUnit(1, ssao.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		else
		{
			glBlitNamedFramebuffer(framebuffer.getHandle(), 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::Begin("Control", nullptr);
		ImGui::Checkbox("Enable SSAO", &g_EnableSSAO);
		// https://github.com/ocornut/imgui/issues/1889#issuecomment-398681105
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !g_EnableSSAO);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * g_EnableSSAO ? 1.0f : 0.2f);
		ImGui::Checkbox("Enable blur", &g_EnableBlur);
		ImGui::SliderFloat("SSAO scale", &g_SSAOParams.scale_, 0.0f, 2.0f);
		ImGui::SliderFloat("SSAO bias", &g_SSAOParams.bias_, 0.0f, 0.3f);
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
		ImGui::Separator();
		ImGui::SliderFloat("SSAO radius", &g_SSAOParams.radius, 0.05f, 0.5f);
		ImGui::SliderFloat("SSAO attenuation scale", &g_SSAOParams.attScale, 0.5f, 1.5f);
		ImGui::SliderFloat("SSAO distance scale", &g_SSAOParams.distScale, 0.0f, 1.0f);
		ImGui::End();
		imguiTextureWindowGL("Color", framebuffer.getTextureColor().getHandle());
		imguiTextureWindowGL("Depth", framebuffer.getTextureDepth().getHandle());
		imguiTextureWindowGL("SSAO", ssao.getTextureColor().getHandle());
		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app_->swapBuffers();

	}

	return 0;
}
