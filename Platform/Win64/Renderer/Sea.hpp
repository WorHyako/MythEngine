#pragma once

#include <RHICommon.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class TestCamera;

/* CubeMap Face identifiers */
typedef enum
{
	CUBEMAP_FACE_POSITIVE_X = 0,
	CUBEMAP_FACE_NEGATIVE_X = 1,
	CUBEMAP_FACE_POSITIVE_Y = 2,
	CUBEMAP_FACE_NEGATIVE_Y = 3,
	CUBEMAP_FACE_POSITIVE_Z = 4,
	CUBEMAP_FACE_NEGATIVE_Z = 5,

	CUBEMAP_FACE_MAX = 0x7fffffff
} CubemapFaces;

struct RS_RECT
{
	glm::vec3 pos;
	float size;
	float angle;
	uint32_t colorPacked;
	uint32_t subTexture;
};

struct RS_SPRITE
{
	glm::vec3 pos;
	uint32_t color;
	glm::vec2 uv;
};

inline void FillSpriteVertex(RS_SPRITE& pV, float x, float y, float z = 1.0f, uint32_t color_ = 0xFFFFFF,
	float tu = 0.0f, float tv = 0.0f)
{
	pV.pos.x = x;
	pV.pos.y = y;
	pV.pos.z = z;
	pV.color = color_;
	pV.uv.s = tu;
	pV.uv.t = tv;
}

struct Plane
{
	glm::vec3 normal = glm::vec3{};
	float distance = 0.0f;

	Plane(const glm::vec3& normal_, const glm::vec3& point)
	{
		normal = normal_;
		distance = glm::dot(point, normal);
	}
};

struct SeaVertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct SeaBlock
{
	int32_t iX1, iX2, iY1, iY2; // result rectangle(in units)
	int32_t iSize0;             //

	int32_t iTX, iTY;
	int32_t iSize;
	int32_t iLOD;
	int32_t iIStart;
	int32_t iIFirst, iILast;

	bool bInProgress, bDone;

	static bool QSort(const SeaBlock& b1, const SeaBlock& b2)
	{
		return (b1.iLOD > b2.iLOD);
	}

	inline bool operator<(const SeaBlock& B) const
	{
		return (B.iLOD < iLOD);
	};
};

struct SeaTrash
{
	glm::vec3 pos;
	glm::vec3 speed;
	uint32_t dwSubTexture;
	float time;
	float size;
};

struct SeaLight
{
	glm::vec3 pos;
	glm::vec3 speed;
	uint32_t dwSubTexture;
	float time;
};

class Sea
{
public:
	Sea();
	virtual ~Sea();

	virtual bool initializeRender();


protected:
	void CreateVertexDeclaration();
	void SFLB_CreateBuffers();
	void BuildVolumeTexture();
	bool EditMode_Update();

	void SSE_WaveXZ(SeaVertex** pArray);
	void SSE_WaveXZBlock(SeaBlock& pB);

	float WaveXZ(float x, float z, glm::vec3* pNormal = nullptr);
	void PrepareIndicesForBlock(uint32_t blockIndex);

	void CalculateNormalMap(float fFrame, float fAmplitude, float* pfOut, std::vector<uint32_t*>& aFrames);
	void CalculateHeightMap(float fFrame, float fAmplitude, float* pfOut, std::vector<uint8_t*>& aFrames);
	bool SunRoad_Render2();
	bool EnvMap_Render2();
	bool SunRoad_Render();
	bool EnvMap_Render();
	void EnvMap_GetSideMatrix(CubemapFaces Face, glm::mat4x4& mView);

	void Realize(float deltaTime);

	void BuildTree(int32_t iTX, int32_t iTY, int32_t iLev);
	void AddBlock(int32_t iTX, int32_t iTY, int32_t iSize, int32_t iLOD);
	void CalculateLOD(const glm::vec3& v1, const glm::vec3& v2, int32_t& iMaxLOD, int32_t& iMinLOD);
	inline float CalcLod(const float& x, const float& y, const float& z);
	bool isVisibleBBox(const glm::vec3& vCenter, const glm::vec3& v1, const glm::vec3& v2);
	int32_t VisCode(const glm::vec3& vP);

	TestCamera* GetCamera() const;

private:
	RHI::ShaderHandle m_VertexShader = nullptr;
	RHI::ShaderHandle m_PixelShader = nullptr;
	RHI::BufferHandle m_ConstantBuffer = nullptr;
	RHI::BufferHandle m_VertexBuffer = nullptr;
	RHI::BufferHandle m_IndexBuffer = nullptr;
	RHI::TextureHandle m_Texture = nullptr;
	RHI::SamplerHandle m_Sampler = nullptr;
	RHI::InputLayoutHandle m_InputLayout = nullptr;
	RHI::BindingLayoutHandle m_BindingLayout = nullptr;
	RHI::BindingSetHandle m_BindingSets[1];
	RHI::GraphicsPipelineHandle m_GraphicsPipeline = nullptr;
	RHI::CommandListHandle m_CommandList = nullptr;
	RHI::DeviceHandle m_Device = nullptr;

	bool m_UnderSea;
	bool m_UnderSeaEnable;
	bool m_UnderSeaStarted;

	RHI::BufferHandle m_VerticesSeaBuffer = nullptr;
	RHI::BufferHandle m_IndicesSeaBuffer = nullptr;

	RHI::TextureHandle pVolumeTexture;
	RHI::TextureHandle pEnvMap, pSunRoadMap;
	RHI::TextureHandle pZStencil;

	RHI::TextureHandle pReflection, pReflectionSunroad;
	RHI::TextureHandle pReflectionSurfaceDepth;

	bool m_Started;
	bool m_SimpleSea;

	SeaVertex* pVSea;
	uint16_t* pTriangles;
	uint32_t* pIndices;
	int32_t verticesStart, trianglesStart, indicesStart;

	std::vector<SeaTrash> aSeaTrash;

	std::vector<SeaLight> aSeaLights;

	std::vector<uint32_t*> aNormals;
	std::vector<uint8_t*> aBumps;
	std::vector<SeaBlock> m_Blocks;

	glm::vec4 v4SeaColor, v4SkyColor, v4SeaParameters;

	std::vector<RHI::TextureHandle> aBumpMaps;
	RHI::TextureHandle pRenderTargetBumpMap;

	float m_FogSeaDensity {};
	float m_FogStartDistance {};
	glm::vec3 m_FogColor = glm::vec3{};
	bool m_FogEnable;

	glm::mat4 m_TexProjection;

	float* pSeaFrame1, * pSeaFrame2, * pSeaNormalsFrame1, * pSeaNormalsFrame2;

	bool m_Stop {};

	std::uint32_t m_MaxDim {};
	std::uint32_t m_MinDim {};
	float m_MaxSeaHeight{};
	float m_MaxSeaDistance {};
	float m_GridStep {};
	float m_LodScale {};
	float m_SeaHeightOffset {};

	float m_BumpScale {};
	float fBumpSpeed = 1.0f;

	float m_LastLightTime {};
	RHI::TextureHandle m_SeaLightTexture = nullptr;
	std::vector<RS_RECT> aLightsRects;

	float m_LastTrashTime {};
	RHI::TextureHandle m_SeaTrashTexture = nullptr;
	std::vector<RS_RECT> aTrashRects;

	float m_FoamV {};
	float m_FoamK {};
	float m_FoamUV {};
	float m_FoamTextureDisturb {};
	bool bIniFoamEnable, bFoamEnable;
	RHI::TextureHandle m_FoamTexture = nullptr;

	float m_PosShift {};
	float m_Frenel;

	glm::vec3 m_WorldOffset = glm::vec3{};
	glm::vec3 m_SeaCenterPos = glm::vec3{};
	glm::vec3 m_CamPos = glm::vec3{};

	Plane* pFrustumPlanes;

	std::vector<RHI::VertexBufferBinding> vertexBufferBindings;

	// debug
	bool m_TempFullMode = true;
	float m_TempGridStep = 0.0f;
	float m_TempLodScale = 0.0f;
};
