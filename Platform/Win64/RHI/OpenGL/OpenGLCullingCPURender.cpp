#include <RHI/OpenGL/OpenGLCullingCPURender.hpp>

#include <RHI/OpenGL/Framework/GLShader.hpp>
#include <RHI/OpenGL/Framework/GLSceneData.hpp>
#include <RHI/OpenGL/Framework/GLFramebuffer.hpp>
#include <RHI/OpenGL/Framework/GLMesh.hpp>
#include <RHI/OpenGL/Framework/UtilsGLImGui.hpp>
#include <RHI/OpenGL/Framework/LineCanvasGL.hpp>
#include <RHI/OpenGL/Framework/GLSkyboxRenderer.hpp>
#include <Utils/UtilsFPS.hpp>

#include <Camera/TestCamera.hpp>
#include <UserInput/GLFW/GLFWUserInput.hpp>

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 light = mat4(0.0f); // unused in this demo
	vec4 cameraPos;
};

mat4 g_CullingView;
bool g_DrawMeshes = true;
bool g_DrawBoxes = true;
bool g_DrawGrid = true;

OpenGLCullingCPURender::OpenGLCullingCPURender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(-10.0f, 3.0f, 3.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{
	g_CullingView = testCamera_->getViewMatrix();
}

int OpenGLCullingCPURender::draw()
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

	const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLBuffer perFrameDataBuffer(kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle(), 0, kUniformBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	GLSceneData sceneData(
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.meshes").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.scene").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "Data/meshes/bistro_all.materials").c_str()
	);

	GLMesh mesh{ sceneData };

	GLSkyboxRenderer skybox(
		(FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k.hdr").c_str(),
		(FilesystemUtilities::GetResourcesDir() + "textures/immenstadter_horn_2k_irradiance.hdr").c_str());
	ImGuiGLRenderer rendererUI;
	CanvasGL canvas;

	// pretransform bounding boxes to world space
	for(const auto& c : sceneData.shapes_)
	{
		const mat4 model = sceneData.scene_.globalTransform_[c.transformIndex];
		sceneData.meshData_.boxes_[c.meshIndex].transform(model);
	}

	const BoundingBox fullScene = combineBoxes(sceneData.meshData_.boxes_);

	FramesPerSecondCounter fpsCounter(0.5f);

	while (!glfwWindowShouldClose(app_->getWindow()))
	{
		fpsCounter.tick(app_->getDeltaSeconds());

		positioner_->update(app_->getDeltaSeconds(), input.mouseState->pos, input.mouseState->pressedLeft);

		int width, height;
		glfwGetFramebufferSize(app_->getWindow(), &width, &height);
		const float ratio = width / (float)height;

		glClearNamedFramebufferfv(0, GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glClearNamedFramebufferfi(0, GL_DEPTH_STENCIL, 0, 1.0f, 0);

		const mat4 proj = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4 view = testCamera_->getViewMatrix();

		const PerFrameData perFrameData{ view, proj, glm::mat4(0.0f), glm::vec4(testCamera_->getPosition(), 1.0f) };
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		if (!input.freezeCullingView)
			g_CullingView = testCamera_->getViewMatrix();

		vec4 frustumPlanes[6];
		getFrustumPlanes(proj * g_CullingView, frustumPlanes);
		vec4 frustumCorners[8];
		getFrustumCorners(proj * g_CullingView, frustumCorners);

		// cull
		int numVisibleMeshes = 0;
		{
			DrawElementsIndirectCommand* cmd = mesh.bufferIndirect_.drawCommands_.data();
			for (const auto& c : sceneData.shapes_)
			{
				cmd->instanceCount_ = isBoxInFrustum(frustumPlanes, frustumCorners, sceneData.meshData_.boxes_[c.meshIndex]) ? 1 : 0;
				numVisibleMeshes += (cmd++)->instanceCount_;
			}
			mesh.bufferIndirect_.uploadIndirectBuffer();
		}

		if(g_DrawBoxes)
		{
			DrawElementsIndirectCommand* cmd = mesh.bufferIndirect_.drawCommands_.data();
			for (const auto& c : sceneData.shapes_)
				drawBox3dGL(canvas, mat4(1.0f), sceneData.meshData_.boxes_[c.meshIndex], (cmd++)->instanceCount_ ? vec4(0, 1, 0, 1) : vec4(1, 0, 0, 1));
			drawBox3dGL(canvas, mat4(1.0f), fullScene, vec4(1, 0, 0, 1));
		}

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

		if(input.freezeCullingView)
			renderCameraFrustumGL(canvas, g_CullingView, proj, vec4(1, 1, 0, 1), 100);
		canvas.flush();

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::Begin("Control", nullptr);
		ImGui::Text("Draw:");
		ImGui::Checkbox("Meshes", &g_DrawMeshes);
		ImGui::Checkbox("Boxes", &g_DrawBoxes);
		ImGui::Checkbox("Grid", &g_DrawGrid);
		ImGui::Separator();
		ImGui::Checkbox("Freeze culling frustum (P)", &input.freezeCullingView);
		ImGui::Separator();
		ImGui::Text("Visible meshes: %i", numVisibleMeshes);
		ImGui::End();
		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app_->swapBuffers();
	}

	return 0;
}
