#include <RHI/OpenGL/OpenGLShadowMapping.hpp>

#include <vector>

#include <RHI/OpenGL/Framework/GLFWApp.hpp>
#include <RHI/OpenGL/Framework/GLShader.hpp>
#include <RHI/OpenGL/Framework/GLTexture.hpp>
#include <RHI/OpenGL/Framework/GLFramebuffer.hpp>
#include <RHI/OpenGL/Framework/LineCanvasGL.hpp>
#include <RHI/OpenGL/Framework/UtilsGLImGui.hpp>
#include <Camera/TestCamera.hpp>

#include "UserInput/GLFW/GLFWUserInput.hpp"

struct PerFrameData
{
	mat4 view;
	mat4 proj;
	mat4 light;
	vec4 cameraPos;
	vec4 lightAngles; // cos(inner), cos(outer)
	vec4 lightPos;
};

bool g_RotateModel = true;

float g_LightAngle = 60.0f;
float g_LightInnerAngle = 10.0f;
float g_LightNear = 1.0f;
float g_LightFar = 20.0f;

float g_LightDist = 12.0f;
float g_LightXAngle = -1.0f;
float g_LightYAngle = -2.0f;

OpenGLShadowMappingRender::OpenGLShadowMappingRender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(0.0f, 6.0f, 11.0f), vec3(0.0f, 4.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{
	
}

int OpenGLShadowMappingRender::draw()
{
	GLFWUserInput& input = GLFWUserInput::GetInput();
	input.positioner = positioner_;
	positioner_->maxSpeed_ = 5.0f;

	GLShader shdGridVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.vert").c_str());
	GLShader shdGridFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.frag").c_str());
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLBuffer perFrameDataBuffer(kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer.getHandle(), 0, kUniformBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLShader shdModelVert((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/SceneModel.vert").c_str());
	GLShader shdModelFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/SceneModel.frag").c_str());
	GLProgram progModel(shdModelVert, shdModelFrag);

	GLShader shdShadowVert((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/ShadowMapping.vert").c_str());
	GLShader shdShadowFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/ShadowMapping.frag").c_str());
	GLProgram progShadowMap(shdShadowVert, shdShadowFrag);

	// 1. Duck
	GLMeshPVP mesh((FilesystemUtilities::GetResourcesDir() + "objects/rubber_duck/scene.gltf").c_str());
	GLTexture texAlbedoDuck(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "objects/rubber_duck/textures/Duck_baseColor.png").c_str());

	// 2. Plane
	const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
	const std::vector<VertexData> vertices = {
		{vec3(-2, -2, 0), vec3(0,0,1), vec2(0,0)},
		{vec3(-2, +2, 0), vec3(0,0,1), vec2(0,1)},
		{vec3(+2, +2, 0), vec3(0,0,1), vec2(1,1)},
		{vec3(+2, -2, 0), vec3(0,0,1), vec2(1,0)},
	};
	GLMeshPVP plane(indices, vertices.data(), uint32_t(sizeof(VertexData) * vertices.size()));
	GLTexture texAlbedoPlane(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "textures/ch2_sample3_STB.jpg").c_str());

	const std::vector<GLMeshPVP*> meshesToDraw = { &mesh, &plane };

	// shadow map
	GLFramebuffer shadowMap(1024, 1024, GL_RGBA8, GL_DEPTH_COMPONENT24);

	// model matrices
	const mat4 m(1.0f);
	GLBuffer modelMatrices(sizeof(mat4), value_ptr(m), GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, modelMatrices.getHandle());

	double timeStamp = glfwGetTime();
	float deltaSeconds = 0.0f;
	float angle = 0.0f;

	ImGuiGLRenderer rendererUI;
	CanvasGL canvas;

	while (!glfwWindowShouldClose(app_->getWindow()))
	{
		positioner_->update(deltaSeconds, input.mouseState->pos, input.mouseState->pressedLeft);

		const double newTimeStamp = glfwGetTime();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize(app_->getWindow(), &width, &height);
		const float ratio = width / (float)height;

		if (g_RotateModel)
			angle += deltaSeconds;

		// 0. Calculate light parameters
		const glm::mat4 rotY = glm::rotate(mat4(1.f), g_LightYAngle, glm::vec3(0, 1, 0));
		const glm::mat4 rotX = glm::rotate(rotY, g_LightXAngle, glm::vec3(1, 0, 0));
		const glm::vec4 lightPos = rotX * glm::vec4(0, 0, g_LightDist, 1.0f);
		const mat4 lightProj = glm::perspective(glm::radians(g_LightAngle), 1.0f, g_LightNear, g_LightFar);
		const mat4 lightView = glm::lookAt(glm::vec3(lightPos), vec3(0), vec3(0, 1, 0));

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		// 1. Render shadow map
		{
			PerFrameData perFrameData{};
			perFrameData.view = lightView;
			perFrameData.proj = lightProj;
			perFrameData.cameraPos = glm::vec4(testCamera_->getPosition(), 1.0f);
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);
			shadowMap.bind();
			glClearNamedFramebufferfv(shadowMap.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			glClearNamedFramebufferfi(shadowMap.getHandle(), GL_DEPTH_STENCIL, 0, 1.0f, 0);
			progShadowMap.useProgram();
			for (const auto& m : meshesToDraw)
				m->drawElements();
			shadowMap.unbind();
		}

		// 2. Render scene
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const mat4 proj = glm::perspective(45.0f, ratio, 0.5f, 5000.0f);
		const mat4 view = testCamera_->getViewMatrix();

		PerFrameData perFrameData{};
		perFrameData.view = view;
		perFrameData.proj = proj;
		perFrameData.light = lightProj * lightView;
		perFrameData.cameraPos = vec4(testCamera_->getPosition(), 1.0f);
		perFrameData.lightAngles = vec4(cosf(glm::radians(0.5f * g_LightAngle)), cosf(glm::radians(0.5f * (g_LightAngle - g_LightInnerAngle))), 1.0f, 1.0f);
		perFrameData.lightPos = lightPos;
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		const mat4 scale = glm::scale(mat4(1.0f), vec3(3.0f));
		const mat4 rot = glm::rotate(mat4(1.0f), glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
		const mat4 pos = glm::translate(mat4(1.0f), vec3(0.0f, 0.0f, +1.0f));
		const mat4 m = glm::rotate(scale * rot * pos, angle, vec3(0.0f, 0.0f, 1.0f));
		glNamedBufferSubData(modelMatrices.getHandle(), 0, sizeof(mat4), glm::value_ptr(m));

		const GLuint textures[] = { texAlbedoDuck.getHandle(), texAlbedoPlane.getHandle() };
		glBindTextureUnit(1, shadowMap.getTextureDepth().getHandle());
		progModel.useProgram();
		for(size_t i = 0; i != meshesToDraw.size(); i++)
		{
			glBindTextureUnit(0, textures[i]);
			meshesToDraw[i]->drawElements();
		}

		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);

		renderCameraFrustumGL(canvas, lightView, lightProj, vec4(0.0f, 1.0f, 0.0f, 1.0f));
		canvas.flush();

		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();

		ImGui::Begin("Control", nullptr);
		ImGui::Checkbox("Rotate model", &g_RotateModel);
		ImGui::Text("Light parameters", nullptr);
		ImGui::SliderFloat("Proj::Light angle", &g_LightAngle, 15.0f, 170.0f);
		ImGui::SliderFloat("Proj::Light inner angle", &g_LightInnerAngle, 1.0f, 15.0f);
		ImGui::SliderFloat("Proj::Near", &g_LightNear, 0.1f, 5.0f);
		ImGui::SliderFloat("Proj::Far", &g_LightFar, 0.1f, 100.0f);
		ImGui::SliderFloat("Pos::Dist", &g_LightDist, 0.5f, 100.0f);
		ImGui::SliderFloat("Pos::AngleX", &g_LightXAngle, -3.15f, +3.15f);
		ImGui::SliderFloat("Pos::AngleY", &g_LightYAngle, -3.15f, +3.15f);
		ImGui::End();

		imguiTextureWindowGL("Color", shadowMap.getTextureColor().getHandle());
		imguiTextureWindowGL("Depth", shadowMap.getTextureDepth().getHandle());

		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		app_->swapBuffers();
	}

	return 0;
}
