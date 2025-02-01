#include <Renderer/SceneRenderer.hpp>
#include <RHIModuleWrapper.hpp>

#include <Filesystem/FilesystemUtilities.hpp>\

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Texture/TextureUtils.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
};

static const Vertex g_Vertices[] = {
	{ {-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f} }, // front face
	{ { 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f} },
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f} },
	{ { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f} },

	{ { 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f} }, // right side face
	{ { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f} },
	{ { 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f} },
	{ { 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f} },

	{ {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f} }, // left side face
	{ {-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f} },
	{ {-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f} },
	{ {-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f} },

	{ { 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f} }, // back face
	{ {-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f} },
	{ { 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f} },
	{ {-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f} },

	{ {-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f} }, // top face
	{ { 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f} },
	{ { 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f} },
	{ {-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f} },

	{ { 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f} }, // bottom face
	{ {-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f} },
	{ { 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f} },
	{ {-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f} },
};

static const uint32_t g_Indices[] = {
	 0,  1,  2,   0,  3,  1, // front face
	 4,  5,  6,   4,  7,  5, // left face
	 8,  9, 10,   8, 11,  9, // right face
	12, 13, 14,  12, 15, 13, // back face
	16, 17, 18,  16, 19, 17, // top face
	20, 21, 22,  20, 23, 21, // bottom face
};

static const glm::vec3 g_Offsets[] = {
	glm::vec3(-0.5f, -0.5f, 0.0f),
	glm::vec3(-0.5f, 0.5f, 0.0f),
	glm::vec3(0.5f, -0.5f, 0.0f),
	glm::vec3(0.5f, 0.5f, 0.0f)
};

// This example uses a single large constant buffer with multiple views to draw multiple versions of the same model.
	// The alignment and size of partially bound constant buffers must be a multiple of 256 bytes,
	// so define a struct that represents one constant buffer entry or slice for one draw call.
struct ConstantBufferEntry
{
	glm::mat4x4 viewProjMatrix;
	float padding[16 * 3];
};

SceneRenderer::SceneRenderer(RHI::IDynamicRHI* dynamicRHI)
	: RendererInterface(dynamicRHI)
{
	
}

SceneRenderer::~SceneRenderer()
{

}

bool SceneRenderer::initializeRender()
{
	if(m_Device)
	{
		m_VertexShader = m_Device->createShaderModule((FilesystemUtilities::GetShadersDir() + "Vulkan/CubeTest.vert").c_str());
		m_PixelShader = m_Device->createShaderModule((FilesystemUtilities::GetShadersDir() + "Vulkan/CubeTest.frag").c_str());

		if(!m_VertexShader || !m_PixelShader)
		{
			return false;
		}

		RHI::BufferDesc constantBufferDesc = {};
		constantBufferDesc
			.setSize(sizeof(ConstantBufferEntry) * c_NumViews)
			.setIsUniformBuffer(true)
			.setIsTransferDst(true);
		m_ConstantBuffer = m_Device->createBuffer(constantBufferDesc);

		RHI::VertexInputAttributeDesc attributes[] = {
			RHI::VertexInputAttributeDesc()
				.setFormat(RHI::Format::RGB32_FLOAT)
				.setOffset(offsetof(Vertex, position))
				.setBinding(0)
				.setLocation(0),
			RHI::VertexInputAttributeDesc()
				.setFormat(RHI::Format::RG32_FLOAT)
				.setOffset(offsetof(Vertex, uv))
				.setBinding(0)
				.setLocation(1)
		};

		RHI::VertexInputBindingDesc bindings[] =
		{
		RHI::VertexInputBindingDesc()
		.setBinding(0)
		.setStride(sizeof(Vertex))
		};
		m_InputLayout = m_Device->createInputLayout(attributes, bindings);

		RHI::CommandListParameters commandListParams = { RHI::CommandQueue::Graphics };
		m_CommandList = m_Device->createCommandList(commandListParams);
		m_CommandList->beginSingleTimeCommands();
		m_CommandLists.push_back(m_CommandList.get());

		RHI::BufferDesc vertexBufferDesc = {};
		vertexBufferDesc
			.setSize(sizeof(g_Vertices))
			.setIsVertexBuffer(true)
			.setIsTransferDst(true);
		m_VertexBuffer = m_Device->createBuffer(vertexBufferDesc);

		//m_CommandList->beginTrackingBufferState(m_VertexBuffer, RHI::ResourceStates::CopyDest);
		m_CommandList->writeBuffer(m_VertexBuffer.get(), sizeof(g_Vertices), g_Vertices);
		//m_CommandList->setPermanentBufferState(m_VertexBuffer, RHI::ResourceStates::VertexBuffer);

		//m_CommandList->endSingleTimeCommands();
		//m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);
		//m_CommandList->queueWaitIdle();

		//m_CommandList->beginSingleTimeCommands();

		RHI::BufferDesc indexBufferDesc;
		indexBufferDesc
			.setSize(sizeof(g_Indices))
			.setIsIndexBuffer(true)
			.setIsTransferDst(true);
		m_IndexBuffer = m_Device->createBuffer(indexBufferDesc);

		//m_CommandList->beginTrackingBufferState(m_IndexBuffer, RHI::ResourceStates::CopyDest);
		m_CommandList->writeBuffer(m_IndexBuffer.get(), sizeof(g_Indices), g_Indices);
		//m_CommandList->setPermanentBufferState(m_IndexBuffer, RHI::ResourceStates::IndexBuffer);

		//m_CommandList->endSingleTimeCommands();
		//m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);
		//m_CommandList->queueWaitIdle();

		//m_CommandList->beginSingleTimeCommands();

		m_Texture = RenderUtils::loadTexture2D(m_Device.get(), m_CommandList.get(), (FilesystemUtilities::GetResourcesDir() + "textures/container2.png").c_str());
		m_Sampler = m_Device->createTextureSampler();

		m_CommandList->endSingleTimeCommands();
		m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);

		// Create a single binding layout and multiple binding sets, one set per view.
		// The different binding sets use different slices of the same constant buffer.
		for (uint32_t viewIndex = 0; viewIndex < c_NumViews; ++viewIndex)
		{
			RHI::BufferAttachment bufferAttachment = {};
			bufferAttachment
				.setDescriptorInfo(RHI::DescriptorInfo{RHI::DescriptorType::UNIFORM_BUFFER, RHI::ShaderStageFlagBits::VERTEX_BIT})
				.setBuffer(m_ConstantBuffer.get())
				.setSize(sizeof(ConstantBufferEntry))
				.setOffset(viewIndex * sizeof(ConstantBufferEntry));

			RHI::TextureAttachment textureAttachment = {};
			textureAttachment
				.setDescriptorInfo(RHI::DescriptorInfo{ RHI::DescriptorType::COMBINED_IMAGE_SAMPLER, RHI::ShaderStageFlagBits::FRAGMENT_BIT })
				.setTexture(m_Texture.get())
				.setSampler(m_Sampler.get());

			RHI::DescriptorSetInfo dsInfos = {.buffers = {bufferAttachment}, .textures = {textureAttachment}};

			m_BindingLayout = m_Device->createDescriptorSetLayout(dsInfos);
			m_BindingSets[viewIndex] = m_Device->createDescriptorSet(dsInfos, 1, m_BindingLayout.get());
		}

		return true;
	}

	return false;
}

void SceneRenderer::updateBuffers()
{
	
}

void SceneRenderer::composeFrame()
{
	
}

bool SceneRenderer::renderScene()
{
	RHI::FramebufferHandle framebuffer = m_DynamicRHI->GetFramebuffer(m_DynamicRHI->GetCurrentBackBufferIndex());

	if(!m_GraphicsPipeline)
	{
		RHI::GraphicsPipelineDesc pipelineDesc;
		pipelineDesc.VS = m_VertexShader;
		pipelineDesc.PS = m_PixelShader;
		pipelineDesc.inputLayout = m_InputLayout;
		pipelineDesc.bindingLayouts = { m_BindingLayout };
		pipelineDesc.primType = RHI::PrimitiveType::TriangleList;
		pipelineDesc.renderState.depthStencilState.depthTestEnable = true;

		m_GraphicsPipeline = m_Device->createGraphicsPipeline(pipelineDesc, framebuffer.get());
	}

    //updateBuffers(imageIndex);

	m_CommandList->beginSingleTimeCommands();

    //composeFrame(imageIndex);

	// Fill out the constant buffer slices for multiple views of the model.
	static uint64_t numFrames = 0;

	if(numFrames == 0)
	{
		ConstantBufferEntry modelConstants[c_NumViews];
		for (uint32_t viewIndex = 0; viewIndex < c_NumViews; ++viewIndex)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, g_Offsets[viewIndex]);
			glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 projection = glm::perspective(glm::radians(60.0f), float(framebuffer->framebufferWidth) / float(framebuffer->framebufferHeight), 0.1f, 10.0f);
			glm::mat4 viewProjMatrix = projection * view * model;
			modelConstants[viewIndex].viewProjMatrix = viewProjMatrix;
		}

		m_CommandList->writeBuffer(m_ConstantBuffer.get(), sizeof(modelConstants), modelConstants);
	}
	numFrames++;

	//m_CommandList->endSingleTimeCommands();
	//m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);
	//m_CommandList->queueWaitIdle();

	//m_CommandList->beginSingleTimeCommands();

	/*RHI::GraphicsState state = {};
	state.pipeline = m_GraphicsPipeline;
	state.framebuffer = m_DynamicRHI->GetFramebuffer(m_DynamicRHI->GetCurrentBackBufferIndex());
	m_CommandList->setGraphicsState(state);

	RHI::DrawArguments drawArgs = {};
	drawArgs.vertexCount = 3;
	m_CommandList->draw(drawArgs);*/

	for (uint32_t viewIndex = 0; viewIndex < c_NumViews; ++viewIndex)
	{
		RHI::GraphicsState state;
		// Pick the right binding set for this view.
		state.bindingSets = { m_BindingSets[viewIndex] };
		state.indexBufferBinding = { m_IndexBuffer.get(), 0, 1 };
		state.vertexBufferBindings = {
			{ m_VertexBuffer.get(), 0, offsetof(Vertex, position) },
			{ m_VertexBuffer.get(), 1, offsetof(Vertex, uv) }
		};
		state.pipeline = m_GraphicsPipeline.get();
		state.framebuffer = framebuffer.get();

		// Update the pipeline, bindings, and other state.
		m_CommandList->setGraphicsState(state);

		// Draw the model.
		RHI::DrawArguments drawArgs = {};
		drawArgs.vertexCount = sizeof(g_Indices) / sizeof(uint32_t);
		m_CommandList->drawIndexed(drawArgs);
	}

	m_CommandList->endSingleTimeCommands();

	m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);

	return true;
}
