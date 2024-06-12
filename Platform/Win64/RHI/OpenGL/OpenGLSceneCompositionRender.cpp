#include <RHI/OpenGL/OpenGLSceneCompositionRender.hpp>

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
	mat4 light;
	vec4 cameraPos;
	vec4 frustumPlanes[6];
	vec4 frustumCorners[8];
	uint32_t numShapesToCull;
};

static_assert(sizeof(SSAOParams) <= sizeof(PerFrameData));

static_assert(sizeof(HDRParams) <= sizeof(PerFrameData));

OpenGLSceneCompositionRender::OpenGLSceneCompositionRender(GLApp* app)
	: OpenGLBaseRender(app)
	, positioner_(new CameraPositioner_FirstPerson(vec3(-10.0f, 3.0f, 3.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, 1.0f, 0.0f)))
	, testCamera_(new TestCamera(*positioner_))
{

}

int OpenGLSceneCompositionRender::draw()
{
	GLFWUserInput& input = GLFWUserInput::GetInput();
	input.positioner = positioner_;
	positioner_->maxSpeed_ = 1.0f;

	// grid
	GLShader shdGridVertex((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.vert").c_str());
	GLShader shdGridFragment((FilesystemUtilities::GetShadersDir() + "OpenGL/InfinityGridShader/InfinityGridShader.frag").c_str());
	GLProgram progGrid(shdGridVertex, shdGridFragment);
	// scene
	GLShader shaderVert((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/SceneIBL.vert").c_str());
	GLShader shaderFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/SceneIBL.frag").c_str());
	GLProgram program(shaderVert, shaderFrag);
	// generic postprocessing
	GLShader shdFullScreenQuadVert((FilesystemUtilities::GetShadersDir() + "OpenGL/FullScreenQuad/FullScreenQuad.vert").c_str());
	// OIT
	GLShader shaderFragOIT((FilesystemUtilities::GetShadersDir() + "OpenGL/OITransparency/MeshOIT.frag").c_str());
	GLProgram programOIT(shaderVert, shaderFragOIT);
	GLShader shdCombineOIT((FilesystemUtilities::GetShadersDir() + "OpenGL/OITransparency/OIT.frag").c_str());
	GLProgram progCombineOIT(shdFullScreenQuadVert, shdCombineOIT);
	// GPU culling
	GLShader shaderCulling((FilesystemUtilities::GetShadersDir() + "OpenGL/FrustumCulling/FrustumCulling.comp").c_str());
	GLProgram programCulling(shaderCulling);
	// SSAO
	GLShader shdSSAOFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/SSAO.frag").c_str());
	GLShader shdCombineSSAOFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/SSAO_Combine.frag").c_str());
	GLProgram progSSAO(shdFullScreenQuadVert, shdSSAOFrag);
	GLProgram progCombineSSAO(shdFullScreenQuadVert, shdCombineSSAOFrag);
	// Blur
	GLShader shdBlurXFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/BlurX.frag").c_str());
	GLShader shdBlurYFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/CookbookSSAO/BlurY.frag").c_str());
	GLProgram progBlurX(shdFullScreenQuadVert, shdBlurXFrag);
	GLProgram progBlurY(shdFullScreenQuadVert, shdBlurYFrag);
	// HDR
	GLShader shdCombineHDR((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/HDR.frag").c_str());
	GLProgram progCombineHDR(shdFullScreenQuadVert, shdCombineHDR);
	GLShader shdToLuminance((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/ToLuminance.frag").c_str());
	GLProgram progToLuminance(shdFullScreenQuadVert, shdToLuminance);
	GLShader shdBrightPass((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/BrightPass.frag").c_str());
	GLProgram progBrightPass(shdFullScreenQuadVert, shdBrightPass);
	GLShader shdAdaptation((FilesystemUtilities::GetShadersDir() + "OpenGL/HDR/Adaptation.comp").c_str());
	GLProgram progAdaptation(shdAdaptation);
	// Shadows
	GLShader shdShadowVert((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/IndirectShadowMapping.vert").c_str());
	GLShader shdShadowFrag((FilesystemUtilities::GetShadersDir() + "OpenGL/ShadowMapping/IndirectShadowMapping.frag").c_str());
	GLProgram progShadowMap(shdShadowVert, shdShadowFrag);

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
	if (!numVisibleMeshesPtr)
	{
		printf("numVisibleMeshesPtr == nullptr\n");
		exit(255);
	}

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
	CanvasGL canvas;
	GLTexture rotationPattern(GL_TEXTURE_2D, (FilesystemUtilities::GetResourcesDir() + "textures/rot_texture.bmp").c_str());

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
	GLFramebuffer opaqueFramebuffer(width, height, GL_RGBA16F, GL_DEPTH_COMPONENT24);
	GLFramebuffer framebuffer(width, height, GL_RGBA16F, GL_DEPTH_COMPONENT24);
	GLFramebuffer luminance(64, 64, GL_RGBA16F, 0);
	GLFramebuffer brightPass(256, 256, GL_RGBA16F, 0);
	GLFramebuffer bloom1(256, 256, GL_RGBA16F, 0);
	GLFramebuffer bloom2(256, 256, GL_RGBA16F, 0);
	GLFramebuffer ssao(1024, 1024, GL_RGBA8, 0);
	GLFramebuffer blur(1024, 1024, GL_RGBA8, 0);
	// create a texture view into the last mip-level (1x1 pixel) of our luminance framebuffer
	GLuint luminance1x1;
	glGenTextures(1, &luminance1x1);
	glTextureView(luminance1x1, GL_TEXTURE_2D, luminance.getTextureColor().getHandle(), GL_RGBA16F, 6, 1, 0, 1);
	// ping-pong textures for light adaptation
	GLTexture luminance1(GL_TEXTURE_2D, 1, 1, GL_RGBA16F);
	GLTexture luminance2(GL_TEXTURE_2D, 1, 1, GL_RGBA16F);
	const GLTexture* luminances[] = { &luminance1, &luminance2 };
	const vec4 brightPixel(vec3(50.0f), 1.0f);
	glTextureSubImage2D(luminance1.getHandle(), 0, 0, 0, 1, 1, GL_RGBA, GL_FLOAT, glm::value_ptr(brightPixel));


	const uint32_t kMaxOITFragments = 16 * 1024 * 1024;
	const GLuint kBufferIndex_TransparencyLists = kBufferIndex_Materials + 1;
	GLBuffer oitAtomicCounter(sizeof(uint32_t), nullptr, GL_DYNAMIC_STORAGE_BIT);
	GLBuffer oitTransparencyLists(sizeof(TransparentFragment)* kMaxOITFragments, nullptr, GL_DYNAMIC_STORAGE_BIT);
	GLTexture oitHeads(GL_TEXTURE_2D, width, height, GL_R32UI);
	glBindImageTexture(0, oitHeads.getHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, oitAtomicCounter.getHandle());

	auto clearTransparencyBuffers = [&oitAtomicCounter, &oitHeads]()
		{
			const uint32_t minusOne = 0xFFFFFFFF;
			const uint32_t zero = 0;
			glClearTexImage(oitHeads.getHandle(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &minusOne);
			glNamedBufferSubData(oitAtomicCounter.getHandle(), 0, sizeof(uint32_t), &zero);
		};

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

	BoundingBox bigBox = reorderedBoxes.front();
	for (const auto& b : reorderedBoxes)
	{
		bigBox.combinePoint(b.min_);
		bigBox.combinePoint(b.max_);
	}

	FramesPerSecondCounter fpsCounter(0.5f);

	auto imGuiPushFlagsAndStyles = [](bool value)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !value);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * value ? 1.0f : 0.2f);
		};
	auto imGuiPopFlagsAndStyles = []()
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		};

	GLFramebuffer shadowMap(8192, 8192, GL_R8, GL_DEPTH_COMPONENT24);
	const GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
	glTextureParameteriv(shadowMap.getTextureColor().getHandle(), GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	glTextureParameteriv(shadowMap.getTextureDepth().getHandle(), GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

	GLsync fenceCulling = nullptr;

	while (!glfwWindowShouldClose(app_->getWindow()))
	{
		fpsCounter.tick(app_->getDeltaSeconds());

		if (sceneData.uploadLoadedTextures())
			mesh.updateMaterialsBuffer(sceneData);

		positioner_->update(app_->getDeltaSeconds(), input.mouseState->pos, input.mouseState->pressedLeft);

		int width, height;
		glfwGetFramebufferSize(app_->getWindow(), &width, &height);
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClearNamedFramebufferfv(opaqueFramebuffer.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
		glClearNamedFramebufferfi(opaqueFramebuffer.getHandle(), GL_DEPTH_STENCIL, 0, 1.0f, 0);

		if (!freezeCullingView)
			cullingView = testCamera_->getViewMatrix();

		const mat4 proj = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
		const mat4 view = testCamera_->getViewMatrix();

		// calculate light parameters for shadow mapping
		const glm::mat4 rot1 = glm::rotate(mat4(1.f), glm::radians(lightTheta), glm::vec3(0, 0, 1));
		const glm::mat4 rot2 = glm::rotate(rot1, glm::radians(lightPhi), glm::vec3(1, 0, 0));
		const vec3 lightDir = glm::normalize(vec3(rot2 * vec4(0.0f, -1.0f, 0.0f, 1.0f)));
		const mat4 lightView = glm::lookAt(glm::vec3(0.0f), lightDir, vec3(0, 0, 1));
		const BoundingBox box = bigBox.getTransformed(lightView);
		const mat4 lightProj = glm::ortho(box.min_.x, box.max_.x, box.min_.y, box.max_.y, -box.max_.z, -box.min_.z);

		PerFrameData perFrameData = {
			view,
			proj,
			lightProj * lightView,
			glm::vec4(testCamera_->getPosition(), 1.0f),
		};

		getFrustumPlanes(proj * cullingView, perFrameData.frustumPlanes);
		getFrustumCorners(proj * cullingView, perFrameData.frustumCorners);

		clearTransparencyBuffers();

		// cull
		{
			*numVisibleMeshesPtr = 0;
			programCulling.useProgram();
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_BoundingBoxes, boundingBoxesBuffer.getHandle());
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_NumVisibleMeshes, numVisibleMeshesBuffer.getHandle());

			perFrameData.numShapesToCull = enableGPUCulling ? (uint32_t)meshesOpaque.drawCommands_.size() : 0u;
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_DrawCommands, meshesOpaque.getHandle());
			glDispatchCompute(1 + (GLuint)meshesOpaque.drawCommands_.size() / 64, 1, 1);

			perFrameData.numShapesToCull = enableGPUCulling ? (uint32_t)meshesTransparent.drawCommands_.size() : 0u;
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_DrawCommands, meshesTransparent.getHandle());
			glDispatchCompute(1 + (GLuint)meshesTransparent.drawCommands_.size() / 64, 1, 1);

			glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
			fenceCulling = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, kBufferIndex_TransparencyLists, oitTransparencyLists.getHandle());

		// 1. Render shadow map
		if (enableShadows)
		{
			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
			// Calculate light parameters
			const PerFrameData perFrameDataShadows = { lightView, lightProj };
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameDataShadows);
			glClearNamedFramebufferfv(shadowMap.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			glClearNamedFramebufferfi(shadowMap.getHandle(), GL_DEPTH_STENCIL, 0, 1.0f, 0);
			shadowMap.bind();
			progShadowMap.useProgram();
			mesh.draw(mesh.bufferIndirect_.drawCommands_.size());
			shadowMap.unbind();
			perFrameData.light = lightProj * lightView;
			glBindTextureUnit(4, shadowMap.getTextureDepth().getHandle());
		}
		else
		{
			// disable shadows
			perFrameData.light = mat4(0.0f);
		}
		glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, kUniformBufferSize, &perFrameData);

		// 1. Render scene
		opaqueFramebuffer.bind();
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		// 1.0 Cube map
		skybox.draw();
		// 1.1 Bistro
		if (drawOpaque)
		{
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
			program.useProgram();
			mesh.draw(meshesOpaque.drawCommands_.size(), &meshesOpaque);
			glDisable(GL_CULL_FACE);

		}
		if (drawGrid)
		{
			glEnable(GL_BLEND);
			progGrid.useProgram();
			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
			glDisable(GL_BLEND);
		}
		if (drawBoxes)
		{
			DrawElementsIndirectCommand* cmd = mesh.bufferIndirect_.drawCommands_.data();
			for (const auto& c : sceneData.shapes_)
				drawBox3dGL(canvas, mat4(1.0f), sceneData.meshData_.boxes_[c.meshIndex], vec4(0, 1, 0, 1));
		}
		drawBox3dGL(canvas, mat4(1.0f), bigBox, vec4(1, 1, 1, 1));
		if (drawTransparent)
		{
			glBindImageTexture(0, oitHeads.getHandle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			programOIT.useProgram();
			mesh.draw(meshesTransparent.drawCommands_.size(), &meshesTransparent);
			glFlush();
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
			glDepthMask(GL_TRUE);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}
		if (freezeCullingView)
			renderCameraFrustumGL(canvas, cullingView, proj, vec4(1, 1, 0, 1), 100);
		if (enableShadows && showLightFrustum)
		{
			renderCameraFrustumGL(canvas, lightView, lightProj, vec4(1, 0, 0, 1), 100);
			canvas.line(vec3(0.0f), lightDir * 100.0f, vec4(0, 0, 1, 1));
		}
		canvas.flush();
		opaqueFramebuffer.unbind();
		// SSAO
		if (enableSSAO)
		{
			glDisable(GL_DEPTH_TEST);
			glClearNamedFramebufferfv(ssao.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, sizeof(SSAOParams), &SSAOParams);
			ssao.bind();
			progSSAO.useProgram();
			glBindTextureUnit(0, opaqueFramebuffer.getTextureDepth().getHandle());
			glBindTextureUnit(1, rotationPattern.getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
			ssao.unbind();
			if (enableBlur)
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
			glClearNamedFramebufferfv(framebuffer.getHandle(), GL_COLOR, 0, glm::value_ptr(vec4(0.0f, 0.0f, 0.0f, 1.0f)));
			framebuffer.bind();
			progCombineSSAO.useProgram();
			glBindTextureUnit(0, opaqueFramebuffer.getTextureColor().getHandle());
			glBindTextureUnit(1, ssao.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
			framebuffer.unbind();
		}
		else
		{
			glBlitNamedFramebuffer(opaqueFramebuffer.getHandle(), framebuffer.getHandle(), 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}
		// combine OIT
		opaqueFramebuffer.bind();
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		progCombineOIT.useProgram();
		glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
		glDrawArrays(GL_TRIANGLES, 0, 6);
		opaqueFramebuffer.unbind();
		glBlitNamedFramebuffer(opaqueFramebuffer.getHandle(), framebuffer.getHandle(), 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		//
		// HDR pipeline
		//
		// pass HDR params to shaders
		if (enableHDR)
		{
			glNamedBufferSubData(perFrameDataBuffer.getHandle(), 0, sizeof(HDRParams), &HDRParams);
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
			glBindImageTexture(0, luminances[0]->getHandle(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
			glBindImageTexture(1, luminance1x1, 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
			glBindImageTexture(2, luminances[1]->getHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
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
				// 2.4 Blur X
				bloom1.bind();
				progBlurX.useProgram();
				glBindTextureUnit(0, bloom2.getTextureColor().getHandle());
				glDrawArrays(GL_TRIANGLES, 0, 6);
				bloom1.unbind();
				// 2.5 Blur Y
				bloom2.bind();
				progBlurY.useProgram();
				glBindTextureUnit(0, bloom1.getTextureColor().getHandle());
				glDrawArrays(GL_TRIANGLES, 0, 6);
				bloom2.unbind();
			}
			// 3. Apply tone mapping
			glViewport(0, 0, width, height);
			progCombineHDR.useProgram();
			glBindTextureUnit(0, framebuffer.getTextureColor().getHandle());
			glBindTextureUnit(1, luminances[1]->getHandle());
			glBindTextureUnit(2, bloom2.getTextureColor().getHandle());
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		else
		{
			glBlitNamedFramebuffer(framebuffer.getHandle(), 0, 0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		}

		// wait for compute shader results to become visible
		if (enableGPUCulling && fenceCulling) {
			for (;;) {
				const GLenum res = glClientWaitSync(fenceCulling, GL_SYNC_FLUSH_COMMANDS_BIT, 1000);
				if (res == GL_ALREADY_SIGNALED || res == GL_CONDITION_SATISFIED) break;
			}
			glDeleteSync(fenceCulling);
		}

		glViewport(0, 0, width, height);

		const float indentSize = 16.0f;
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((float)width, (float)height);
		ImGui::NewFrame();
		ImGui::Begin("Control", nullptr);
		ImGui::Text("Transparency:");
		ImGui::Indent(indentSize);
		ImGui::Checkbox("Opaque meshes", &drawOpaque);
		ImGui::Checkbox("Transparent meshes", &drawTransparent);
		ImGui::Unindent(indentSize);
		ImGui::Separator();
		ImGui::Text("GPU culling:");
		ImGui::Indent(indentSize);
		ImGui::Checkbox("Enable GPU culling", &enableGPUCulling);
		imGuiPushFlagsAndStyles(enableGPUCulling);
		ImGui::Checkbox("Freeze culling frustum (P)", &freezeCullingView);
		ImGui::Text("Visible meshes: %i", *numVisibleMeshesPtr);
		imGuiPopFlagsAndStyles();
		ImGui::Unindent(indentSize);
		ImGui::Separator();
		ImGui::Text("SSAO:");
		ImGui::Indent(indentSize);
		ImGui::Checkbox("Enable SSAO", &enableSSAO);
		imGuiPushFlagsAndStyles(enableSSAO);
		ImGui::Checkbox("Enable SSAO blur", &enableBlur);
		ImGui::SliderFloat("SSAO scale", &SSAOParams.scale_, 0.0f, 2.0f);
		ImGui::SliderFloat("SSAO bias", &SSAOParams.bias_, 0.0f, 0.3f);
		ImGui::SliderFloat("SSAO radius", &SSAOParams.radius, 0.02f, 0.2f);
		ImGui::SliderFloat("SSAO attenuation scale", &SSAOParams.attScale, 0.5f, 1.5f);
		ImGui::SliderFloat("SSAO distance scale", &SSAOParams.distScale, 0.0f, 1.0f);
		imGuiPopFlagsAndStyles();
		ImGui::Unindent(indentSize);
		ImGui::Separator();
		ImGui::Text("HDR:");
		ImGui::Indent(indentSize);
		ImGui::Checkbox("Enable HDR", &enableHDR);
		imGuiPushFlagsAndStyles(enableHDR);
		ImGui::SliderFloat("Exposure", &HDRParams.exposure_, 0.1f, 2.0f);
		ImGui::SliderFloat("Max white", &HDRParams.maxWhite_, 0.5f, 2.0f);
		ImGui::SliderFloat("Bloom strength", &HDRParams.bloomStrength_, 0.0f, 2.0f);
		ImGui::SliderFloat("Adaptation speed", &HDRParams.adaptationSpeed_, 0.01f, 0.5f);
		imGuiPopFlagsAndStyles();
		ImGui::Unindent(indentSize);
		ImGui::Separator();
		ImGui::Text("Shadows:");
		ImGui::Indent(indentSize);
		ImGui::Checkbox("Enable shadows", &enableShadows);
		imGuiPushFlagsAndStyles(enableShadows);
		ImGui::Checkbox("Show light's frustum (red) and scene AABB (white)", &showLightFrustum);
		ImGui::SliderFloat("Light Theta", &lightTheta, -85.0f, +85.0f);
		ImGui::SliderFloat("Light Phi", &lightPhi, -85.0f, +85.0f);
		imGuiPopFlagsAndStyles();
		ImGui::Unindent(indentSize);
		ImGui::Separator();
		ImGui::Checkbox("Grid", &drawGrid);
		ImGui::Checkbox("Bounding boxes (all)", &drawBoxes);
		ImGui::End();
		if (enableSSAO)
			imguiTextureWindowGL("SSAO", ssao.getTextureColor().getHandle());
		if (enableShadows)
			imguiTextureWindowGL("Shadow Map", shadowMap.getTextureDepth().getHandle());
		ImGui::Render();
		rendererUI.render(width, height, ImGui::GetDrawData());

		// swap current and adapter luminances
		std::swap(luminances[0], luminances[1]);

		app_->swapBuffers();
	}

	glUnmapNamedBuffer(numVisibleMeshesBuffer.getHandle());
	glDeleteTextures(1, &luminance1x1);

	return 0;
}
