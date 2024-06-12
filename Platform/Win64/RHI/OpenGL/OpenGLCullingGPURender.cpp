#include <RHI/OpenGL/OpenGLCullingGPURender.hpp>

#include <RHI/OpenGL/Framework/GLShader.hpp>
#include <RHI/OpenGL/Framework/GLSceneData.hpp>
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
	vec4 frustumPlanes[6];
	vec4 frustumCorners[8];
	uint32_t numShapesToCull;
};

OpenGLCullingGPURender::OpenGLCullingGPURender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(-10.0f, 3.0f, 3.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{
	cullingView = testCamera_->getViewMatrix();
}

int OpenGLCullingGPURender::draw()
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

	GLShader shaderCulling((FilesystemUtilities::GetShadersDir() + "OpenGL/FrustumCulling/FrustumCulling.comp").c_str());
	GLProgram programCulling(shaderCulling);

	const GLuint kMaxNumObjects = 128 * 1024;
	const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);
	const GLsizeiptr kBoundingBoxesBufferSize = sizeof(BoundingBox) * kMaxNumObjects;
	const GLuint kBufferIndex_BoundingBoxes = kBufferIndex_PerFrameUniforms + 1;
	const GLuint kBufferIndex_DrawCommands = kBufferIndex_PerFrameUniforms + 2;
	const GLuint kBufferIndex_NumVisibleMeshes = kBufferIndex_PerFrameUniforms + 3;

	GLBuffer perFrameDataBuffer(kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle(), 0, kUniformBufferSize);
	GLBuffer boundingBoxesBuffer(kBoundingBoxesBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	GLBuffer numVisibleMeshesBuffer(sizeof(uint32_t), nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
	volatile uint32_t* numVisibleMeshesPtr = (uint32_t*)glMapNamedBuffer(numVisibleMeshesBuffer.getHandle(), GL_READ_WRITE);
	assert(numVisibleMeshesPtr);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	GLSceneData sceneData(
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.materials").c_str()
	);

	GLMesh mesh(sceneData);

	GLSkyboxRenderer skybox(
		(FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k.hdr").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k_irradiance.hdr").c_str());
	ImGuiGLRenderer rendererUI;
	CanvasGL canvas;

	bool g_DrawMeshes = true;
	bool g_DrawGrid = true;

	std::vector<BoundingBox> reorderedBoxes;
	reorderedBoxes.reserve(sceneData.shapes_.size());

	// pretransform bounding boxes to world space
	for (const auto& c : sceneData.shapes_)
	{
		const mat4 model = sceneData.scene_.globalTransform_[c.transformIndex];
		reorderedBoxes.push_back(sceneData.meshData_.boxes_[c.meshIndex]);
		reorderedBoxes.back().transform(model);
	}
	glNamedBufferSubData(boundingBoxesBuffer.getHandle(), 0, reorderedBoxes.size() * sizeof(BoundingBox), reorderedBoxes.data());

	FramesPerSecondCounter fpsCounter(0.5f);

	while (!glfwWindowShouldClose(app_->getWindow()))
	{
		fpsCounter.tick(app_->getDeltaSeconds());

		positioner_->update(app_->getDeltaSeconds(), input.mouseState->pos, input.mouseState->pressedLeft);

		int width, height;
		glfwGetFramebufferSize(app_->getWindow(), &width, &height);
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClearNamedFramebufferfv(0, GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glClearNamedFramebufferfi(0, GL_DEPTH_STENCIL, 0, 1.0f, 0);

		if (!input.freezeCullingView)
			cullingView = testCamera_->getViewMatrix();

		const mat4 proj = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4 view = testCamera_->getViewMatrix();
		PerFrameData perFrameData{};
		perFrameData.view = view;
		perFrameData.proj = proj;
		perFrameData.light = mat4(0.0f);
		perFrameData.cameraPos = glm::vec4(testCamera_->getPosition(), 1.0f);
		perFrameData.numShapesToCull = enableGPUCulling ? (uint32_t)sceneData.shapes_.size() : 0u;

		getFrustumPlanes(proj * cullingView, perFrameData.frustumPlanes);
		getFrustumCorners(proj * cullingView, perFrameData.frustumCorners);

		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		// cull
		*numVisibleMeshesPtr = 0;
		programCulling.useProgram();
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_BoundingBoxes, boundingBoxesBuffer.getHandle());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_DrawCommands, mesh.bufferIndirect_.getHandle());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_NumVisibleMeshes, numVisibleMeshesBuffer.getHandle());
		glDispatchCompute(1 + (GLuint)sceneData.shapes_.size() / 64, 1, 1);
		glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
		const GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

		// 1. Render scene
		skybox.draw();
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		// 1.1 Bistro
		if (g_DrawMeshes)
		{
			program.useProgram();
			mesh.draw(sceneData.shapes_.size());
		}

		// 1.2 Grid
		glEnable(GL_BLEND);

		if (g_DrawGrid)
		{
			progGrid.useProgram();
			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
		}

		if (input.freezeCullingView)
			renderCameraFrustumGL(canvas, cullingView, proj, vec4(1, 1, 0, 1), 100);
		canvas.flush();

		// wait for compute shader results to become visible
		for (;;)
		{
			const GLenum res = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000);
			if(res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED) break;
		}
		glDeleteSync(fence);

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::Begin("Control", nullptr);
		ImGui::Text("Draw:");
		ImGui::Checkbox("Meshes", &g_DrawMeshes);
		ImGui::Checkbox("Grid", &g_DrawGrid);
		ImGui::Separator();
		ImGui::Checkbox("Enable GPU culling", &enableGPUCulling);
		ImGui::Checkbox("Freeze culling frustum (P)", &input.freezeCullingView);
		ImGui::Separator();
		ImGui::Text("Visible meshes: %i", *numVisibleMeshesPtr);
		ImGui::End();
		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app_->swapBuffers();
	}

	glUnmapNamedBuffer(numVisibleMeshesBuffer.getHandle());

	return 0;
}
