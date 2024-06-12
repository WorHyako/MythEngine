#include <RHI/OpenGL/OpenGLOITransparency.hpp>

#include <RHI/OpenGL/Framework/GLShader.hpp>
#include <RHI/OpenGL/Framework/GLSceneDataLazy.hpp>
#include <RHI/OpenGL/Framework/GLFramebuffer.hpp>
#include <RHI/OpenGL/Framework/GLMesh.hpp>
#include <RHI/OpenGL/Framework/UtilsGLImGui.hpp>
#include <RHI/OpenGL/Framework/LineCanvasGL.hpp>
#include <RHI/OpenGL/Framework/GLSkyboxRenderer.hpp>

#include <Camera/TestCamera.hpp>
#include <UserInput/GLFW/GLFWUserInput.hpp>
#include <Utils/UtilsFPS.hpp>

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 light = mat4(0.0f); // unused in this demo
	vec4 cameraPos;
};

OpenGLOITransparencyRender::OpenGLOITransparencyRender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(-10.0f, 3.0f, 3.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{

}

int OpenGLOITransparencyRender::draw()
{
	GLFWUserInput& input = GLFWUserInput::GetInput();
	input.positioner = positioner_;
	positioner_->maxSpeed_ = 1.0f;

	GLShader shdGridVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.vert").c_str());
	GLShader shdGridFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.frag").c_str());
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	GLShader shaderVert((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/SceneIBL.vert").c_str());
	GLShader shaderFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/SceneIBL.frag").c_str());
	GLProgram program(shaderVert, shaderFrag);

	GLShader shaderFragOIT((FilesystemUtilities::GetShadersDir() + "OpenGL/OITransparency/MeshOIT.frag").c_str());
	GLProgram programOIT(shaderVert, shaderFragOIT);

	GLShader shdFullScreenQuadVert((FilesystemUtilities::GetShadersDir() + "OpenGL/FullScreenQuad/FullScreenQuad.vert").c_str());
	GLShader shdCombineOIT((FilesystemUtilities::GetShadersDir() + "OpenGL/OITransparency/OIT.frag").c_str());
	GLProgram progCombineOIT(shdFullScreenQuadVert, shdCombineOIT);

	const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);
	GLBuffer perFrameDataBuffer(kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle(), 0, kUniformBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	GLSceneDataLazy sceneData(
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.materials").c_str()
	);
	GLMesh mesh(sceneData);

	GLSkyboxRenderer skybox(
		(FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k.hdr").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k_irradiance.hdr").c_str());
	ImGuiGLRenderer rendererUI;

	GLIndirectBuffer meshesOpaque(sceneData.shapes_.size());
	GLIndirectBuffer meshesTransparent(sceneData.shapes_.size());

	auto isTransparent = [&sceneData](const DrawElementsIndirectCommand& c)
		{
			const auto mtlIndex = c.baseInstance_ & 0xffff;
			const auto& mtl = sceneData.materials_[mtlIndex];
			return (mtl.flags_ & sMaterialFlags_Transparent) > 0;
		};

	mesh.bufferIndirect_.selectTo(meshesOpaque, [&isTransparent](const DrawElementsIndirectCommand& c) -> bool
		{
			return !isTransparent(c);
		});
	mesh.bufferIndirect_.selectTo(meshesTransparent, [&isTransparent](const DrawElementsIndirectCommand& c) -> bool
		{
			return isTransparent(c);
		});

	struct TransparentFragment
	{
		float R, G, B, A;
		float Depth;
		uint32_t Next;
	};

	int width, height;
	glfwGetFramebufferSize(app_->getWindow(), &width, &height);
	GLFramebuffer framebuffer(width, height, GL_RGBA8, GL_DEPTH_COMPONENT24);

	const uint32_t kMaxOITFragments = 16 * 1024 * 1024; // 32 * 1024 * 1024
	const uint32_t kBufferIndex_TransparencyLists = kBufferIndex_Materials + 1;
	GLBuffer oitAtomicCounter(sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);
	GLBuffer oitTransparencyLists(sizeof(TransparentFragment) * kMaxOITFragments, nullptr, GL_DYNAMIC_STORAGE_BIT);
	GLTexture oitHeads(GL_TEXTURE_2D, width, height, GL_R32UI);
	glBindImageTexture(0, oitHeads.getHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, oitAtomicCounter.getHandle());
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_TransparencyLists, oitTransparencyLists.getHandle());

	auto clearTransparencyBuffers = [&oitAtomicCounter, &oitHeads]()
		{
			const uint32_t minusOne = 0xFFFFFFFF;
			const uint32_t zero = 0;
			glClearTexImage(oitHeads.getHandle(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &minusOne);
			glNamedBufferSubData(oitAtomicCounter.getHandle(), 0, sizeof(uint32_t), &zero);
		};

	FramesPerSecondCounter fpsCounter(0.5f);

	while (!glfwWindowShouldClose(app_->getWindow()))
	{
		if (sceneData.uploadLoadedTextures())
			mesh.updateMaterialsBuffer(sceneData);

		fpsCounter.tick(app_->getDeltaSeconds());

		positioner_->update(app_->getDeltaSeconds(), input.mouseState->pos, input.mouseState->pressedLeft);

		int width, height;
		glfwGetFramebufferSize(app_->getWindow(), &width, &height);
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClearNamedFramebufferfv(framebuffer.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glClearNamedFramebufferfi(framebuffer.getHandle(), GL_DEPTH_STENCIL, 0, 1.0f, 0);

		const mat4 proj = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4 view = testCamera_->getViewMatrix();

		const PerFrameData perFrameData{ view, proj, glm::mat4(0.0f), glm::vec4(testCamera_->getPosition(), 1.0f) };
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		clearTransparencyBuffers();

		// 1. Render scene
		framebuffer.bind();
		skybox.draw();
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		// 1.0 Cube map
		// 1.1 Bistro
		if (drawOpaque)
		{
			program.useProgram();
			mesh.draw(meshesOpaque.drawCommands_.size(), &meshesOpaque);
		}
		if (drawGrid)
		{
			glEnable(GL_BLEND);
			progGrid.useProgram();
			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
			glDisable(GL_BLEND);
		}
		if(drawTransparent)
		{
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			programOIT.useProgram();
			mesh.draw(meshesTransparent.drawCommands_.size(), &meshesTransparent);
			glFlush();
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glDepthMask(GL_TRUE);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
		framebuffer.unbind();
		// combine
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		progCombineOIT.useProgram();
		glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::Begin("Control", nullptr);
		ImGui::Text("Draw:");
		ImGui::Checkbox("Opaque meshes", &drawOpaque);
		ImGui::Checkbox("Transparent meshes", &drawTransparent);
		ImGui::Checkbox("Grid", &drawGrid);
		ImGui::End();
		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app_->swapBuffers();
	}

	return 0;
}
