#include <RHI/OpenGL/OpenGLLargeSceneRender.hpp>

#include <RHI/OpenGL/Framework/GLSceneData.hpp>
#include <RHI/OpenGL/Framework/GLMesh.hpp>

#include "Camera/TestCamera.hpp"
#include "Filesystem/FilesystemUtilities.hpp"
#include "UserInput/GLFW/GLFWUserInput.hpp"

struct PerFrameData
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 cameraPos;
};

OpenGLLargeSceneRender::OpenGLLargeSceneRender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(-10.0f, 3.0f, 3.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{

}

int OpenGLLargeSceneRender::draw()
{
	GLFWUserInput& input = GLFWUserInput::GetInput();
	input.positioner = positioner_;

	GLShader shdGridVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.vert").c_str());
	GLShader shdGridFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.frag").c_str());
	GLProgram progGrid(shdGridVertex, shdGridFragment);

	const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLBuffer perFrameDataBuffer(kUniformBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, kBufferIndex_PerFrameUniforms, perFrameDataBuffer.getHandle(), 0, kUniformBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	GLShader shaderVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/LargeSceneShader/LargeSceneShader.vert").c_str());
	GLShader shaderFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/LargeSceneShader/LargeSceneShader.frag").c_str());
	GLProgram program(shaderVertex, shaderFragment);

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

	while (!glfwWindowShouldClose(app_->getWindow()))
	{
		positioner_->update(deltaSeconds, input.mouseState->pos, input.mouseState->pressedLeft);

		const double newTimeStamp = glfwGetTime();
		deltaSeconds = static_cast<float>(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		int width, height;
		glfwGetFramebufferSize(app_->getWindow(), &width, &height);
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4 view = testCamera_->getViewMatrix();

		const PerFrameData perFrameData{ view, p, glm::vec4(testCamera_->getPosition(), 1.0f) };
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		glDisable(GL_BLEND);
		program.useProgram();
		mesh1.draw(sceneData1);
		mesh2.draw(sceneData2);

		glEnable(GL_BLEND);
		progGrid.useProgram();
		glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);

		app_->swapBuffers();
	}

	return 0;
}
