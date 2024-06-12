#include <RHI/OpenGL/OpenGLHDRRender.hpp>

#include <RHI/OpenGL/Framework/GLFramebuffer.hpp>
#include <RHI/OpenGL/Framework/GLSceneData.hpp>
#include <RHI/OpenGL/Framework/GLMesh.hpp>
#include <RHI/OpenGL/Framework/UtilsGLImGui.hpp>

#include "Camera/TestCamera.hpp"
#include "Filesystem/FilesystemUtilities.hpp"
#include "UserInput/GLFW/GLFWUserInput.hpp"

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
};

struct HDRParams
{
	float exposure_ = 0.9f;
	float maxWhite_ = 1.17f;
	float bloomStrength_ = 1.1f;
	float adaptationSpeed_ = 0.1;
} g_HDRParams;

static_assert(sizeof(HDRParams) <= sizeof(PerFrameData));

bool g_EnableHDR = true;

OpenGLHDRRender::OpenGLHDRRender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(-15.81f, 5.18f, -5.81f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{
	
}

int OpenGLHDRRender::draw()
{
	GLFWUserInput& input = GLFWUserInput::GetInput();
	input.positioner = positioner_;
	positioner_->maxSpeed_ = 1.0f;

	GLShader shdGridVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.vert").c_str());
	GLShader shdGridFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.frag").c_str());
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	GLShader shdFullScreenQuadVert((FilesystemUtilities::GetShadersDir() + "OpenGL/FullScreenQuad/FullScreenQuad.vert").c_str());

	GLShader shdCombineHDR((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/HDR.frag").c_str());
	GLProgram progCombineHDR(shdFullScreenQuadVert, shdCombineHDR);

	GLShader shdBlurXFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/BlurX.frag").c_str());
	GLShader shdBlurYFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/BlurY.frag").c_str());
	GLProgram progBlurX(shdFullScreenQuadVert, shdBlurXFrag);
	GLProgram progBlurY(shdFullScreenQuadVert, shdBlurYFrag);

	GLShader shdToLuminance((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/ToLuminance.frag").c_str());
	GLProgram progToLuminance(shdFullScreenQuadVert, shdToLuminance);

	GLShader shdBrightPass((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/BrightPass.frag").c_str());
	GLProgram progBrightPass(shdFullScreenQuadVert, shdBrightPass);

	GLShader shdAdaptation((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/Adaptation.comp").c_str());
	GLProgram progAdaptation(shdAdaptation);

	GLShader shaderVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/SceneIBL.vert").c_str());
	GLShader shaderFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/SceneIBL.frag").c_str());
	GLProgram program(shaderVertex, shaderFragment);

	const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLBuffer perFrameDataBuffer(kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle(), 0, kUniformBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

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
	GLFramebuffer framebuffer(width, height, GL_RGBA16F, GL_DEPTH_COMPONENT24);
	GLFramebuffer luminance(64, 64, GL_RGBA16F, 0);
	//GLFramebuffer luminance(64, 64, GL_R16F, 0);
	GLFramebuffer brightPass(256, 256, GL_RGBA16F, 0);
	GLFramebuffer bloom1(256, 256, GL_RGBA16F, 0);
	GLFramebuffer bloom2(256, 256, GL_RGBA16F, 0);

	// create a texture view into the last mip-level (1x1 pixel) of our luminance framebuffer
	GLuint luminance1x1;
	glGenTextures(1, &luminance1x1);
	glTextureView(luminance1x1, GL_TEXTURE_2D, luminance.getTextureColor().getHandle(), GL_RGBA16F, 6, 1, 0, 1);
	//glTextureView(luminance1x1, GL_TEXTURE_2D, luminance.getTextureColor().getHandle(), GL_R16F, 6, 1, 0, 1);
	//const GLint Mask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
	//glTextureParameteriv(luminance1x1, GL_TEXTURE_SWIZZLE_RGBA, Mask);
	// ping-pong textures for light adaptation
	GLTexture luminance1(GL_TEXTURE_2D, 1, 1, GL_RGBA16F);
	GLTexture luminance2(GL_TEXTURE_2D, 1, 1, GL_RGBA16F);
	const GLTexture* luminances[] = { &luminance1, &luminance2 };
	const vec4 brightPixel(vec3(50.0f), 1.0f);
	glTextureSubImage2D(luminance1.getHandle(), 0, 0, 0, 1, 1, GL_RGBA, GL_FLOAT, glm::value_ptr(brightPixel));

	// cube map
	// https://hdrihaven.com/hdri/?h=immenstadter_horn
	GLTexture envMap(GL_TEXTURE_CUBE_MAP, (FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k.hdr").c_str());
	GLTexture envMapIrradiance(GL_TEXTURE_CUBE_MAP, (FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k_irradiance.hdr").c_str());

	GLShader shdCubeVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/Cube.vert").c_str());
	GLShader shdCubeFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/Cube.frag").c_str());
	GLProgram progCube(shdCubeVertex, shdCubeFragment);

	GLuint dummyVAO;
	glCreateVertexArrays(1, &dummyVAO);

	const GLuint pbrTextures[] = { envMap.getHandle(), envMapIrradiance.getHandle() };
	glBindTextures(5, 2, pbrTextures);

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

		const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4 view = testCamera_->getViewMatrix();

		const PerFrameData perFrameData{ view, p, glm::vec4(testCamera_->getPosition(), 1.0f) };
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		// 1. Render scene
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		framebuffer.bind();
		// 1.0 Cube map
		progCube.useProgram();
		glBindTextureUnit(1, envMap.getHandle());
		glDepthMask(false);
		glBindVertexArray(dummyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthMask(true);
		// 1.1 Bistro
		program.useProgram();
		mesh1.draw(sceneData1);
		mesh2.draw(sceneData2);
		// 1.2 Grid
		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
		framebuffer.unbind();
		// HDR adaptation start
		glGenerateTextureMipmap(framebuffer.getTextureColor().getHandle());
		glTextureParameteri(framebuffer.getTextureColor().getHandle(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		// HDR adaptation end

		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);

		// pass HDR params to shaders
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, sizeof(g_HDRParams), &g_HDRParams);

		// 2.1 Downscale and convert to luminance
		luminance.bind();
		progToLuminance.useProgram();
		glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);
		luminance.unbind();
		glGenerateTextureMipmap(luminance.getTextureColor().getHandle());

		// 2.2 Light adaptation
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		progAdaptation.useProgram();
		// Either way is possible. In this case, all access modes will be GL_READ_WRITE
#if 0
		const GLuint imageTextures[] = { luminances[0]->getHandle(), luminance1x1, luminances[1]->getHandle() };
		glBindImageTextures(0, 3, imageTextures);
#else
		glBindImageTexture(0, luminances[0]->getHandle(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
		glBindImageTexture(1, luminance1x1, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
		glBindImageTexture(2, luminances[1]->getHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
#endif
		glDispatchCompute(1, 1, 1);
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

		// 2.3 Extract bright areas
		brightPass.bind();
		progBrightPass.useProgram();
		glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);
		brightPass.unbind();

		glBlitNamedFramebuffer(brightPass.getHandle(), bloom2.getHandle(), 0, 0, 256, 256, 0, 0, 256, 256, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		for (int i = 0; i != 4; i++)
		{
			// 2.3 Blur X
			bloom1.bind();
			progBlurX.useProgram();
			glBindTextureUnit(0, bloom2.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
			bloom1.unbind();
			// 2.4 Blur Y
			bloom2.bind();
			progBlurY.useProgram();
			glBindTextureUnit(0, bloom1.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
			bloom2.unbind();
		}

		// 3. Apply tone mapping
		glViewport(0, 0, width, height);

		if(g_EnableHDR)
		{
			progCombineHDR.useProgram();
			glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
			glBindTextureUnit(1, luminances[1]->getHandle());
			//glBindTextureUnit(1, luminance1x1);
			glBindTextureUnit(2, bloom2.getTextureColor().getHandle());
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
		ImGui::Checkbox("Enable HDR", &g_EnableHDR);
		// https://github.com/ocornut/imgui/issues/1889#issuecomment-398681105
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !g_EnableHDR);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * g_EnableHDR ? 1.0f : 0.2f);
		ImGui::Separator();
		ImGui::Text("Average luminance:");
		ImGui::Image((void*)(intptr_t)luminance1x1, ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Text("Adapted luminance:");
		ImGui::Image((void*)(intptr_t)luminances[1]->getHandle(), ImVec2(128, 128), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		ImGui::Separator();
		ImGui::SliderFloat("Exposure", &g_HDRParams.exposure_, 0.1f, 2.0f);
		ImGui::SliderFloat("Max White", &g_HDRParams.maxWhite_, 0.5f, 2.0f);
		ImGui::SliderFloat("Bloom strength", &g_HDRParams.bloomStrength_, 0.0f, 2.0f);
		ImGui::SliderFloat("Adaptation speed", &g_HDRParams.adaptationSpeed_, 0.01f, 0.5f);
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
		ImGui::End();
		imguiTextureWindowGL("Color", framebuffer.getTextureColor().getHandle());
		imguiTextureWindowGL("Luminance", luminance.getTextureColor().getHandle());
		imguiTextureWindowGL("Bright Pass", brightPass.getTextureColor().getHandle());
		imguiTextureWindowGL("Bloom", bloom2.getTextureColor().getHandle());
		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app_->swapBuffers();

		// swap current and adapter luminances
		std::swap(luminances[0], luminances[1]);
	}

	glDeleteTextures(1, &luminance1x1);

	return 0;
}
