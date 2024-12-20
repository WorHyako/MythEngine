#include <RHI/Vulkan/VulkanSceneRenderer.hpp>
#include <RHI/RHICommon/RHIModuleWrapper.hpp>

#include <Filesystem/FilesystemUtilities.hpp>\

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

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

constexpr uint32_t c_NumViews = 4;

// This example uses a single large constant buffer with multiple views to draw multiple versions of the same model.
	// The alignment and size of partially bound constant buffers must be a multiple of 256 bytes,
	// so define a struct that represents one constant buffer entry or slice for one draw call.
struct ConstantBufferEntry
{
	glm::mat4x4 viewProjMatrix;
	float padding[16 * 3];
};

VulkanSceneRenderer::VulkanSceneRenderer(RHI::IDynamicRHI* dynamicRHI)
	: RendererInterface(dynamicRHI)
{
	
}

VulkanSceneRenderer::~VulkanSceneRenderer()
{

}

bool VulkanSceneRenderer::initializeRender()
{
	if(m_Device)
	{
		m_VertexShader = m_Device->createShaderModule((FilesystemUtilities::GetShadersDir() + "Vulkan/VK01.vert").c_str());
		m_PixelShader = m_Device->createShaderModule((FilesystemUtilities::GetShadersDir() + "Vulkan/VK01.frag").c_str());

		if(!m_VertexShader || !m_PixelShader)
		{
			return false;
		}

		RHI::BufferDesc constantBufferDesc = {};
		constantBufferDesc
			.setSize(sizeof(ConstantBufferEntry) * c_NumViews)
			.setIsUniformBuffer(true);
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
		RHI::IInputLayout* inputLayout = m_Device->createInputLayout(attributes, bindings);

		RHI::CommandListParameters commandListParams = { RHI::CommandQueue::Graphics };
		m_CommandList = m_Device->createCommandList(commandListParams);
		m_CommandLists.push_back(m_CommandList);

		RHI::BufferDesc vertexBufferDesc = {};
		vertexBufferDesc
			.setSize(sizeof(g_Vertices))
			.setIsVertexBuffer(true)
			.setIsTransferDst(true);
		m_VertexBuffer = m_Device->createBuffer(vertexBufferDesc);

		m_CommandList->beginTrackingBufferState(m_VertexBuffer, nvrhi::ResourceStates::CopyDest);
		m_CommandList->writeBuffer(m_VertexBuffer, g_Vertices, sizeof(g_Vertices));
		m_CommandList->setPermanentBufferState(m_VertexBuffer, nvrhi::ResourceStates::VertexBuffer);

		RHI::BufferDesc indexBufferDesc;
		indexBufferDesc
			.setSize(sizeof(g_Vertices))
			.setIsIndexBuffer(true)
			.setIsTransferDst(true);
		m_IndexBuffer = m_Device->createBuffer(indexBufferDesc);

		m_CommandList->beginTrackingBufferState(m_IndexBuffer, nvrhi::ResourceStates::CopyDest);
		m_CommandList->writeBuffer(m_IndexBuffer, g_Indices, sizeof(g_Indices));
		m_CommandList->setPermanentBufferState(m_IndexBuffer, nvrhi::ResourceStates::IndexBuffer);

		m_Texture = RenderUtils::loadTexture2D(m_Device, m_CommandList, );

		m_CommandList->endSingleTimeCommands();
		m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);

		// Create a single binding layout and multiple binding sets, one set per view.
		// The different binding sets use different slices of the same constant buffer.
		for (uint32_t viewIndex = 0; viewIndex < c_NumViews; ++viewIndex)
		{
			RHI::BindingSetDesc bindingSetDesc;
			bindingSetDesc.bindings = {
				// Note: using viewIndex to construct a buffer range.
				nvrhi::BindingSetItem::ConstantBuffer(0, m_ConstantBuffer, nvrhi::BufferRange(sizeof(ConstantBufferEntry) * viewIndex, sizeof(ConstantBufferEntry))),
				// Texutre and sampler are the same for all model views.
				nvrhi::BindingSetItem::Texture_SRV(0, m_Texture),
				nvrhi::BindingSetItem::Sampler(0, commonPasses.m_AnisotropicWrapSampler)
			};

			// Create the binding layout (if it's empty -- so, on the first iteration) and the binding set.
			if (!nvrhi::utils::CreateBindingSetAndLayout(GetDevice(), nvrhi::ShaderType::All, 0, bindingSetDesc, m_BindingLayout, m_BindingSets[viewIndex]))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void VulkanSceneRenderer::updateBuffers()
{
	
}

void VulkanSceneRenderer::composeFrame()
{
	
}

bool VulkanSceneRenderer::renderScene()
{
	if(!m_GraphicsPipeline)
	{
		RHI::GraphicsPipelineDesc pipelineDesc;
		pipelineDesc.VS = m_VertexShader;
		pipelineDesc.PS = m_PixelShader;
		pipelineDesc.primType = RHI::PrimitiveType::TriangleList;
		pipelineDesc.pipelineInfo.useDepth = true;

		m_GraphicsPipeline = m_Device->createGraphicsPipeline(pipelineDesc, m_DynamicRHI->GetFramebuffer(m_DynamicRHI->GetCurrentBackBufferIndex()));
	}

    //updateBuffers(imageIndex);

	m_CommandList->beginSingleTimeCommands();

    //composeFrame(imageIndex);

	// Fill out the constant buffer slices for multiple views of the model.
	ConstantBufferEntry modelConstants[c_NumViews];

	//m_CommandList->

	RHI::GraphicsState state = {};
	state.pipeline = m_GraphicsPipeline;
	state.framebuffer = m_DynamicRHI->GetFramebuffer(m_DynamicRHI->GetCurrentBackBufferIndex());
	m_CommandList->setGraphicsState(state);

	RHI::DrawArguments drawArgs = {};
	drawArgs.vertexCount = 3;
	m_CommandList->draw(drawArgs);
    m_CommandList->endSingleTimeCommands();

    m_Device->executeCommandLists(m_CommandLists, m_CommandLists.size(), RHI::CommandQueue::Graphics);

	return true;
}
