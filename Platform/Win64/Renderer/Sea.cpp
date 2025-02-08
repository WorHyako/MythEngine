#include <Renderer/Sea.hpp>

#include <RHI/RenderUtils/Texture/TextureUtils.hpp>

#include <Camera/TestCamera.hpp>
#include <Utils/UtilsMath.hpp>

#include <cmath>
#include <cstdlib>
#include <thread>
#include <algorithm>
#include <chrono>

#include <xmmintrin.h>
#include <Renderer/sse.h>

#include "Filesystem/FilesystemUtilities.hpp"

#define NUM_VERTICES 65500
#define NUM_INDICES 165000

#define FRAMES 64
#define XWIDTH 128
#define YWIDTH 128

#define MIPSLVLS 4

#define ARGB(a, r, g, b) (uint32_t(b) | (uint32_t(g) << 8L) | (uint32_t(r) << 16L) | (uint32_t(a) << 24L))

template<typename T>
inline T Sqr(const T x) {
    return x * x;
}

// Fast floor
inline int32_t ffloor(float f)
{
    return _mm_cvtss_si32(_mm_add_ss(_mm_load_ss(&f), _mm_set_ss(-0.5f)));
}

// Fast conversion of a floating point number to an integer with rounding to the nearest
inline int32_t fftoi(float f)
{
    return _mm_cvtss_si32(_mm_load_ss(&f));
}

// Fast conversion of a floating point number to an integer with discarding of the fractional part
inline int32_t fftol(float f)
{
    return _mm_cvttss_si32(_mm_load_ss(&f));
}

// Random number
inline float RRnd(float min, float max)
{
    return min + (max - min) * std::rand();
}

float maxSeaDistance = 1600.0f;
float gridStep = 0.06f;
float seaHeightOffset = 0.0f;

struct WaveParams
{
    float amplitude = 1.0f;
    float frame = 0.0f;
    float animSpeed = 1.0f;
    glm::vec3 move = glm::vec3(0.0f);
    glm::vec3 moveSpeed = glm::vec3(0.0f);
    float scale = 0.4f;
    float _amplitude = 0.0f;
};

WaveParams wave1{ 1.0f, 0.0f, 1.0f, glm::vec3(0.0f), glm::vec3(5.0f, 0.0f, 0.0f), 0.4f};
WaveParams wave2{1.0f, 10.0f, 1.0f, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 2.0f), 2.0f};

struct ConstantBuffer
{
    glm::mat4 mvp; //16
    glm::vec4 constant2;
    glm::vec4 shadowConst;
    glm::vec4 animation;
    glm::vec4 cameraPos;
    glm::vec4 seaParameters;
    glm::vec4 seaColor;
    glm::vec4 skyColor;
    glm::vec4 vec6;
    glm::vec4 vec7;
    glm::vec4 frenelK;
    glm::vec4 frenelMax;
    //float padding[0];
};

struct ConstantAlphaBuffer
{
    float Alpha;
};

Sea::Sea()
{
    m_Blocks.reserve(128);
    aSeaTrash.reserve(512);
    aTrashRects.reserve(512);
    aSeaLights.reserve(512);
    aLightsRects.reserve(512);

    m_MaxDim = 65536 * 2;
    m_MinDim = 128;

    m_MaxSeaHeight = 20.0f;
    m_MaxSeaDistance = 1600.0f;
    m_GridStep = 0.06f;
    m_LodScale = 0.4f;
    m_SeaHeightOffset = 0.0f;

    m_BumpScale = 0.1f;
    fBumpSpeed = 1.0f;

    m_PosShift = 1.2f;
    m_Frenel = 0.75f;

    m_FoamV = 3.0f;
    m_FoamK = 0.0f;
    m_FoamUV = 3.0f;
    m_FoamTextureDisturb = 0.7f;
    bIniFoamEnable = false;
    bFoamEnable = false;

    m_LastLightTime = 0.0f;

    m_LastTrashTime = 0.0f;

    m_FogSeaDensity = 0.0f;
    m_FogStartDistance = 0.0f;
    m_FogColor = glm::vec3(0.0f);
    m_FogEnable = false;

    pSeaFrame1 = new float[XWIDTH * YWIDTH];
    pSeaFrame2 = new float[XWIDTH * YWIDTH];
    pSeaNormalsFrame1 = new float[2 * XWIDTH * YWIDTH];
    pSeaNormalsFrame2 = new float[2 * XWIDTH * YWIDTH];

    m_WorldOffset = glm::vec3{ 0.0f };
    m_SeaCenterPos = glm::vec3{ 0.0f };

    m_UnderSea = false;
    m_UnderSeaEnable = false;
    m_UnderSeaStarted = false;
    m_Started = false;
    m_SimpleSea = false;

    m_Stop = false;
}

Sea::~Sea()
{
    pReflection = nullptr;
    pReflectionSunroad = nullptr;
    pEnvMap = nullptr;
    pSunRoadMap = nullptr;
    pZStencil = nullptr;
    pReflectionSurfaceDepth = nullptr;
    pVolumeTexture = nullptr;
    pRenderTargetBumpMap = nullptr;

    m_SeaTrashTexture = nullptr;
    m_SeaLightTexture = nullptr;
    m_FoamTexture = nullptr;

    m_VerticesSeaBuffer = nullptr;
    m_IndicesSeaBuffer = nullptr;

    delete[] pIndices;
    delete[] pVSea;

    for (int32_t i = 0; i < aBumpMaps.size(); i++)
        aBumpMaps[i] = nullptr;

    for (int32_t i = 0; i < aBumps.size(); i++)
        delete aBumps[i];
    for (int32_t i = 0; i < aNormals.size(); i++)
        delete aNormals[i];

    delete[] pSeaFrame1;
    delete[] pSeaFrame2;
    delete[] pSeaNormalsFrame1;
    delete[] pSeaNormalsFrame2;
}

bool Sea::initializeRender()
{
    CreateVertexDeclaration();

    {
        //auto config = Config::Load(Constants::ConfigNames::engine());
        //std::ignore = config.SelectSection("sea");
        //bIniFoamEnable = config.Get<std::int64_t>("FoamEnable", 1) != 0;
    }

    m_FoamTexture = RenderUtils::loadTexture2D(m_Device.get(), m_CommandList.get(), (FilesystemUtilities::GetResourcesDir() + "StormResources/Weather/Sea/Pena/pena.tga").c_str()) ;

    RHI::TextureDesc textureDesc{};
    textureDesc.setWidth(XWIDTH)
        .setHeight(YWIDTH)
        .setMipLevels(MIPSLVLS)
        .setFormat(RHI::Format::RGBA8_UNORM)
        .setIsRenderTarget(true)
        .setMemoryProperties(RHI::MemoryPropertiesBits::DEVICE_LOCAL_BIT);

    pRenderTargetBumpMap = m_Device->createImage(textureDesc);

    SFLB_CreateBuffers();

    {
        RHI::TextureDesc desc{};
        desc.setWidth(XWIDTH)
            .setHeight(YWIDTH)
			.setDepth(FRAMES)
            .setMipLevels(4)
			.setDimension(RHI::TextureDimension::Texture3D)
            .setFormat(RHI::Format::RGBA8_UNORM)
            .setIsRenderTarget(true)
            .setMemoryProperties(RHI::MemoryPropertiesBits::HOST_VISIBLE_BIT | RHI::MemoryPropertiesBits::HOST_CACHED_BIT);

        pVolumeTexture = m_Device->createImage(desc);
    }

    {
        RHI::TextureDesc desc{};
        desc.setWidth(128)
            .setHeight(128)
            .setMipLevels(1)
            .setDimension(RHI::TextureDimension::TextureCube)
            .setFormat(RHI::Format::B5G6R5_UNORM)
            .setIsRenderTarget(true)
            .setMemoryProperties(RHI::MemoryPropertiesBits::DEVICE_LOCAL_BIT);

        pEnvMap = m_Device->createImage(desc);
        pSunRoadMap = m_Device->createImage(desc);

        desc.setDimension(RHI::TextureDimension::Texture2D)
            .setFormat(RHI::Format::D24S8)
            .setSampleCount(1);

        pZStencil = m_Device->createImage(desc);
    }

    {
        RHI::TextureDesc desc{};
        desc.setWidth(128)
            .setHeight(128)
            .setMipLevels(1)
            .setFormat(RHI::Format::B5G6R5_UNORM)
            .setIsRenderTarget(true)
            .setMemoryProperties(RHI::MemoryPropertiesBits::DEVICE_LOCAL_BIT);

        pReflection = m_Device->createImage(desc);
        pReflectionSunroad = m_Device->createImage(desc);

        desc.setFormat(RHI::Format::D24S8)
            .setSampleCount(1);

        pReflectionSurfaceDepth = m_Device->createImage(desc);
    }

    m_SeaTrashTexture = RenderUtils::loadTexture2D(m_Device.get(), m_CommandList.get(), (FilesystemUtilities::GetResourcesDir() + "StormResources/SeaTrash.tga").c_str());
    m_SeaLightTexture = RenderUtils::loadTexture2D(m_Device.get(), m_CommandList.get(), (FilesystemUtilities::GetResourcesDir() + "StormResources/SeaLight.tga").c_str());

    uint8_t bMin = 0xFF;
    uint8_t bMax = 0;

    std::vector<uint8_t*> aTmpBumps;

    uint32_t i;

    for (i = 0; i < FRAMES; i++)
    {
        char str[256];
        char* pFBuffer = nullptr;
        uint32_t dwSize;
        sprintf_s(str, "resource\\sea\\sea%.4d.tga", i);
        // sprintf_s(str, "resource\\sea\\sea0000.tga", i);
        fio->LoadFile(str, &pFBuffer, &dwSize);
        if (!pFBuffer)
        {
            core.Trace("Sea: Can't load %s", str);
            return false;
        }

        auto* pFB = pFBuffer + sizeof(TGA_H);

        auto* pBuffer = new uint8_t[XWIDTH * YWIDTH];
        aTmpBumps.push_back(pBuffer);

        for (uint32_t y = 0; y < YWIDTH; y++)
            for (uint32_t x = 0; x < XWIDTH; x++)
            {
                const uint8_t bB = (*pFB);
                // bB = byte(float(bB - 79.0f) * 255.0f / (139.0f - 79.0f));
                if (bB < bMin)
                    bMin = bB;
                if (bB > bMax)
                    bMax = bB;
                pBuffer[x + y * XWIDTH] = bB & 0xFF;
                pFB += sizeof(uint32_t);
            }

        delete[] pFBuffer;
    }

    for (i = 0; i < FRAMES; i++)
    {
        auto* pBuffer = new uint8_t[XWIDTH * YWIDTH];
        aBumps.push_back(pBuffer);

        for (uint32_t y = 0; y < YWIDTH; y++)
            for (uint32_t x = 0; x < XWIDTH; x++)
            {
                const auto dwAddress = x + y * YWIDTH;
                float b1, b2, b3, b4, b5; // -2 -1 0 1 2

                b1 = 0.08f * static_cast<float>(aTmpBumps[(i - 2) & (FRAMES - 1)][dwAddress]);
                b2 = 0.17f * static_cast<float>(aTmpBumps[(i - 1) & (FRAMES - 1)][dwAddress]);
                b3 = 0.50f * static_cast<float>(aTmpBumps[(i - 0) & (FRAMES - 1)][dwAddress]);
                b4 = 0.17f * static_cast<float>(aTmpBumps[(i + 1) & (FRAMES - 1)][dwAddress]);
                b5 = 0.08f * static_cast<float>(aTmpBumps[(i + 2) & (FRAMES - 1)][dwAddress]);

                pBuffer[dwAddress] = aTmpBumps[(i - 0) & (FRAMES - 1)][dwAddress];
            }
    }

    for (i = 0; i < aTmpBumps.size(); i++)
        delete aTmpBumps[i];

    BuildVolumeTexture();

    EditMode_Update();

    return true;
}

void Sea::SFLB_CreateBuffers()
{
    {
        RHI::BufferDesc desc{};
        desc.setSize(NUM_VERTICES * sizeof(SeaVertex))
            .setIsVertexBuffer(true)
            .setMemoryProperties(RHI::MemoryPropertiesBits::HOST_VISIBLE_BIT | RHI::MemoryPropertiesBits::HOST_CACHED_BIT);

        m_VerticesSeaBuffer = m_Device->createBuffer(desc);
    }

    {
        RHI::BufferDesc desc{};
        desc.setSize(NUM_INDICES * 3 * sizeof(uint16_t))
            .setIsIndexBuffer(true)
            .setMemoryProperties(RHI::MemoryPropertiesBits::HOST_VISIBLE_BIT | RHI::MemoryPropertiesBits::HOST_CACHED_BIT);

        m_IndicesSeaBuffer = m_Device->createBuffer(desc);
    }

    pIndices = new uint32_t[NUM_VERTICES * 3];
    pVSea = new SeaVertex[NUM_VERTICES];
}

void Sea::CreateVertexDeclaration()
{
    if (m_VerticesSeaBuffer.get() == nullptr)
        return;

    vertexBufferBindings = {
        { m_VerticesSeaBuffer.get(), 0, offsetof(SeaVertex, position) },
        { m_VerticesSeaBuffer.get(), 1, offsetof(SeaVertex, normal) },
        { m_VerticesSeaBuffer.get(), 2, offsetof(SeaVertex, uv) }
    };
}

void Sea::BuildVolumeTexture()
{
    std::vector<glm::vec3*> vectors;
    uint32_t i, j;

    for (const auto& normal : aNormals)
        delete normal;
    aNormals.clear();

    D3DLOCKED_BOX box[4];

    if (pVolumeTexture)
        for (i = 0; i < 4; i++)
            pVolumeTexture->LockBox(i, &box[i], nullptr, 0);

    for (i = 0; i < aBumpMaps.size(); i++)
        aBumpMaps[i] = nullptr;
    aBumpMaps.clear();

    uint32_t dwTexelSize = 4;
    auto* pDst = static_cast<char*>(new char[XWIDTH * YWIDTH * dwTexelSize]);

    // build normals

    vectors.reserve(FRAMES);
    for (i = 0; i < FRAMES; i++)
    {
        auto pBuffer = new uint32_t[XWIDTH * YWIDTH];
        aNormals.push_back(pBuffer);

        auto* pVectors = new glm::vec3[XWIDTH * YWIDTH];
        vectors.push_back(pVectors);

        for (uint32_t y = 0; y < YWIDTH; y++)
            for (uint32_t x = 0; x < XWIDTH; x++)
            {
#define GET_MASSIVE(dx, dy)                                                                                            \
    (float(pMassive[((x + dx) & (XWIDTH - 1)) + ((y + dy) & (XWIDTH - 1)) * XWIDTH] & 0xFF) / 255.0f)
                uint8_t* pMassive = aBumps[i];

                float fCenter = GET_MASSIVE(0, 0);

                float fLeft = GET_MASSIVE(-1, 0) - fCenter;
                float fRight = GET_MASSIVE(1, 0) - fCenter;
                float fTop = GET_MASSIVE(0, -1) - fCenter;
                float fBottom = GET_MASSIVE(0, 1) - fCenter;

                glm::vec3 vRes, vRes1, d0, d1, d2, d3;

                uint32_t dwNums = 0;
                if (fLeft < 0.0f)
                    dwNums++;
                if (fRight < 0.0f)
                    dwNums++;
                if (fTop < 0.0f)
                    dwNums++;
                if (fBottom < 0.0f)
                    dwNums++;

                float d = 1.0f;
                d0 = glm::vec3(-1.f, d * fLeft, 0.f);
                d1 = glm::vec3(0.f, d * fTop, -1.f);
                d2 = glm::vec3(1.f, d * fRight, 0.f);
                d3 = glm::vec3(0.f, d * fBottom, 1.f);

                // res = !((d0^d1) + (d2^d3));

                glm::vec3 v1 = glm::cross(d1, d0);
                glm::vec3 v2 = glm::cross(d3, d2);
                glm::vec3 v3 = glm::cross(d0, d3);
                glm::vec3 v4 = glm::cross(d2, d1);

                vRes = vRes1 = glm::normalize(glm::cross(d1, d0) + glm::cross(d3, d2) + glm::cross(d0, d3) + glm::cross(d2, d1));
                if (m_SimpleSea)
                {
                    vRes1 = glm::normalize(vRes * glm::vec3(100.0f, 1.0f, 100.0f));
                }
                // CVECTOR vRes1 = !(vRes * CVECTOR(100.0f, 1.0f, 100.0f));
                uint32_t dwRes = MAKELONG(static_cast<short>(vRes.x * 32767.5f), static_cast<short>(vRes.z * 32767.5f));

                aNormals[i][x + y * XWIDTH] = dwRes;
                vectors[i][x + y * XWIDTH] = vRes1;

                int32_t red = fftol((vRes1.x * 0.5f + 0.5f) * 255.0f);   // FIX-ME no ftol
                int32_t green = fftol((vRes1.y * 0.5f + 0.5f) * 255.0f); // FIX-ME no ftol
                int32_t blue = fftol((vRes1.z * 0.5f + 0.5f) * 255.0f);  // FIX-ME no ftol

                if (pVolumeTexture)
                {
                    if (m_SimpleSea)
                        *(uint32_t*)&(
                            static_cast<char*>(box[0].pBits)[i * box[0].SlicePitch + y * box[0].RowPitch + x * 4]) =
                        ARGB(0x80, blue, blue, red);
                    else
                        *(uint32_t*)&(
                            static_cast<char*>(box[0].pBits)[i * box[0].SlicePitch + y * box[0].RowPitch + x * 4]) =
                        ARGB(0x80, blue, green, red);
                }
            }

        if (!pVolumeTexture)
        {
            D3DSURFACE_DESC d3dsd;
            D3DLOCKED_RECT d3dlr;

            aBumpMaps.push_back(nullptr);
            // pBumpMap = &aBumpMaps[aBumpMaps.Add()];
            RHI::TextureHandle* pBumpMap = &aBumpMaps.back();

            RHI::TextureDesc desc{};
            desc.setWidth(XWIDTH)
                .setHeight(YWIDTH)
                .setMipLevels(MIPSLVLS)
                .setFormat(RHI::Format::RGBA8_UNORM)
                .setIsRenderTarget(true)
                .setMemoryProperties(RHI::MemoryPropertiesBits::HOST_VISIBLE_BIT | RHI::MemoryPropertiesBits::HOST_CACHED_BIT);

            // generate mip levels for random bump
            for (uint32_t lev = 0; lev < MIPSLVLS; lev++)
            {
                (*pBumpMap)->GetLevelDesc(lev, &d3dsd);
                (*pBumpMap)->LockRect(lev, &d3dlr, nullptr, 0);

                auto* pDstT = (uint32_t*)pDst;
                for (uint32_t y = 0; y < d3dsd.Height; y++)
                {
                    for (uint32_t x = 0; x < d3dsd.Width; x++)
                    {
                        glm::vec3 vTmp = glm::vec3(0.0f);
                        int32_t dwMult = 1 << (lev);
                        for (uint32_t y1 = y * dwMult; y1 < (y + 1) * dwMult; y1++)
                            for (uint32_t x1 = x * dwMult; x1 < (x + 1) * dwMult; x1++)
                                vTmp += vectors[i][(x1 & (XWIDTH - 1)) + (y1 & (YWIDTH - 1)) * XWIDTH];

                        vTmp *= (1.0f / static_cast<float>(dwMult * dwMult));

                        int red = static_cast<int>((vTmp.x * 0.5f + 0.5f) * 255.0f);   // FIX-ME no ftol
                        int green = static_cast<int>((vTmp.y * 0.5f + 0.5f) * 255.0f); // FIX-ME no ftol
                        int blue = static_cast<int>((vTmp.z * 0.5f + 0.5f) * 255.0f);  // FIX-ME no ftol
                        *pDstT++ = ARGB(255, blue, green, red);
                    }
                }

                // simple copy
                auto pDstTemp = static_cast<uint8_t*>(d3dlr.pBits);
                for (uint32_t y = 0; y < d3dsd.Height; y++)
                {
                    memcpy(pDstTemp, &pDst[y * d3dsd.Width * dwTexelSize], d3dsd.Width * dwTexelSize);
                    pDstTemp += static_cast<uint32_t>(d3dlr.Pitch);
                }

                (*pBumpMap)->UnlockRect(lev);
            }
        }
    }

    for (j = 1; j < 4; j++)
    {
        auto* pVectors = new glm::vec3[(XWIDTH >> j) * (YWIDTH >> j)];

        for (i = 0; i < (FRAMES >> j); i++)
        {
            // calculate current pVectors
            for (uint32_t y = 0; y < (YWIDTH >> j); y++)
                for (uint32_t x = 0; x < (XWIDTH >> j); x++)
                {
                    int32_t iNumVectors = 0;
                    glm::vec3 vVec = glm::vec3(0.0f);
                    uint32_t dwW = 1 << j;
                    for (uint32_t k = i * dwW; k < (i + 1) * dwW; k++)
                    {
                        for (uint32_t yy = y * dwW; yy < (y + 1) * dwW; yy++)
                            for (uint32_t xx = x * dwW; xx < (x + 1) * dwW; xx++)
                            {
                                iNumVectors++;
                                vVec += vectors[k][xx + yy * XWIDTH];
                            }
                    }
                    pVectors[x + y * (XWIDTH >> j)] = vVec / static_cast<float>(iNumVectors);
                }

            //
            if (pVolumeTexture)
                for (uint32_t y = 0; y < (YWIDTH >> j); y++)
                    for (uint32_t x = 0; x < (XWIDTH >> j); x++)
                    {
                        int32_t red = fftol((pVectors[x + y * (XWIDTH >> j)].x * 0.5f + 0.5f) * 255.0f); // FIX-ME no ftol
                        int32_t green = fftol((pVectors[x + y * (XWIDTH >> j)].y * 0.5f + 0.5f) * 255.0f);
                        // FIX-ME no ftol
                        int32_t blue = fftol((pVectors[x + y * (XWIDTH >> j)].z * 0.5f + 0.5f) * 255.0f); // FIX-ME no ftol

                        if (m_SimpleSea)
                            *(uint32_t*)&(static_cast<char*>(
                                box[j].pBits)[i * box[j].SlicePitch + y * box[j].RowPitch + x * 4]) =
                            ARGB(0x80, blue, blue, red);
                        else
                            *(uint32_t*)&(static_cast<char*>(
                                box[j].pBits)[i * box[j].SlicePitch + y * box[j].RowPitch + x * 4]) =
                            ARGB(0x80, blue, green, red);
                    }
        }
        delete[] pVectors;
    }

    if (pVolumeTexture)
        for (i = 0; i < 4; i++)
            pVolumeTexture->UnlockBox(i);

    for (const auto& vector : vectors)
        delete vector;
    vectors.clear();

    delete[] pDst;
}

bool Sea::EditMode_Update()
{
    v4SeaColor = glm::vec4(0, 255, 0, 1.0f);
    v4SkyColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    const float fReflection = 0.8f;
    const float fTransparency = 0.7f;
    const float fFrenel = 0.75f;
    const float fAttenuation = 0.9f;
    v4SeaParameters = glm::vec4(fAttenuation, fReflection, fTransparency, 0.0f);

    wave1.amplitude = wave1._amplitude = 8.0f;
    wave1.animSpeed = 6.0f;
    wave1.scale = 0.5f;
    wave1.moveSpeed = glm::vec3(5.0f, 0.0f, 0.0f);

    wave2.amplitude = wave2._amplitude = 1.0f;
    wave2.animSpeed = 3.0f;
    wave2.scale = 2.0f;
    wave2.moveSpeed = glm::vec3(0.0f, 0.0f, 2.0f);

    m_BumpScale = 0.1f;
    fBumpSpeed = 1.0f;
    m_GridStep = 0.07f;
    m_LodScale = 0.5f;

    m_PosShift = 1.2f;

    Realize(0);

    return true;
}

int32_t Sea::VisCode(const glm::vec3& vP)
{
    int32_t vc = 0;

    // Max - so the accuracy is much higher, blocks should not disappear sometimes in the distance
    Plane* p = &pFrustumPlanes[0];
    if (p->normal.x * vP.x + p->normal.y * vP.y + p->normal.z * vP.z < p->distance)
        vc |= 0x01;
    p = &pFrustumPlanes[1];
    if (p->normal.x * vP.x + p->normal.y * vP.y + p->normal.z * vP.z < p->distance)
        vc |= 0x02;
    p = &pFrustumPlanes[2];
    if (p->normal.x * vP.x + p->normal.y * vP.y + p->normal.z * vP.z < p->distance)
        vc |= 0x04;
    p = &pFrustumPlanes[3];
    if (p->normal.x * vP.x + p->normal.y * vP.y + p->normal.z * vP.z < p->distance)
        vc |= 0x08;

    return vc;
}

bool Sea::isVisibleBBox(const glm::vec3& vCenter, const glm::vec3& v1, const glm::vec3& v2)
{
    // check box visible
    int32_t vc = 0xFF;
    vc &= VisCode(glm::vec3(v1.x, v1.y, v1.z));
    if (vc == 0)
        return true;
    vc &= VisCode(glm::vec3(v1.x, v2.y, v1.z));
    vc &= VisCode(glm::vec3(v2.x, v1.y, v1.z));
    vc &= VisCode(glm::vec3(v2.x, v2.y, v1.z));
    if (vc == 0)
        return true;
    vc &= VisCode(glm::vec3(v1.x, v1.y, v2.z));
    vc &= VisCode(glm::vec3(v1.x, v2.y, v2.z));
    vc &= VisCode(glm::vec3(v2.x, v1.y, v2.z));
    vc &= VisCode(glm::vec3(v2.x, v2.y, v2.z));

    return vc == 0;
}

inline float Sea::CalcLod(const float& x, const float& y, const float& z)
{
    return Sqr(x - m_CamPos.x) + /*Sqr((y - vCamPos.y)) + */ Sqr(z - m_CamPos.z);
}

void Sea::CalculateLOD(const glm::vec3& v1, const glm::vec3& v2, int32_t& iMaxLOD, int32_t& iMinLOD)
{
    float fCur, fMax, fMin;

    fCur = CalcLod(v1.x, m_SeaCenterPos.y, v1.z);
    fMax = fCur;
    fCur = CalcLod(v2.x, m_SeaCenterPos.y, v1.z);
    if (fCur > fMax)
        fMax = fCur;
    fCur = CalcLod(v1.x, m_SeaCenterPos.y, v2.z);
    if (fCur > fMax)
        fMax = fCur;
    fCur = CalcLod(v2.x, m_SeaCenterPos.y, v2.z);
    if (fCur > fMax)
        fMax = fCur;

    if (m_CamPos.x < v1.x)
    {
        if (m_CamPos.z < v1.z)
            fMin = CalcLod(v1.x, m_SeaCenterPos.y, v1.z);
        else if (m_CamPos.z > v2.z)
            fMin = CalcLod(v1.x, m_SeaCenterPos.y, v2.z);
        else
            fMin = CalcLod(v1.x, m_SeaCenterPos.y, m_CamPos.z);
    }
    else if (m_CamPos.x > v2.x)
    {
        if (m_CamPos.z < v1.z)
            fMin = CalcLod(v2.x, m_SeaCenterPos.y, v1.z);
        else if (m_CamPos.z > v2.z)
            fMin = CalcLod(v2.x, m_SeaCenterPos.y, v2.z);
        else
            fMin = CalcLod(v2.x, m_SeaCenterPos.y, m_CamPos.z);
    }
    else
    {
        if (m_CamPos.z < v1.z)
            fMin = CalcLod(m_CamPos.x, m_SeaCenterPos.y, v1.z);
        else if (m_CamPos.z > v2.z)
            fMin = CalcLod(m_CamPos.x, m_SeaCenterPos.y, v2.z);
        else
            fMin = CalcLod(m_CamPos.x, m_SeaCenterPos.y, m_CamPos.z);
    }

    iMaxLOD = static_cast<int>(0.5f * logf(m_LodScale * fMax) / logf(2));
    if (iMaxLOD < 4)
        iMaxLOD = 4;
    iMinLOD = static_cast<int>(0.5f * logf(m_LodScale * fMin) / logf(2));
    if (iMinLOD < 4)
        iMinLOD = 4;
}

void Sea::AddBlock(int32_t iTX, int32_t iTY, int32_t iSize, int32_t iLOD)
{
    m_Blocks.emplace_back(SeaBlock{ 0, 0, 0, 0, iSize >> iLOD, iTX, iTY, iSize, iLOD, 0, 0, 0, false, false });
}

void Sea::BuildTree(int32_t iTX, int32_t iTY, int32_t iLev)
{
    int32_t iMaxLOD, iMinLOD;
    const int32_t iSize = static_cast<int32_t>(m_MaxDim >> iLev);
    const float fGSize = m_GridStep * iSize;

    const glm::vec3 v1 = m_SeaCenterPos + glm::vec3(iTX * fGSize, -m_MaxSeaHeight / 2.0f, iTY * fGSize);
    const glm::vec3 v2 = v1 + glm::vec3(fGSize, m_MaxSeaHeight, fGSize);

    if (!isVisibleBBox(m_SeaCenterPos, v1, v2))
        return;

    CalculateLOD(v1, v2, iMaxLOD, iMinLOD);

    if (iSize <= static_cast<int32_t>(m_MinDim) || iMaxLOD - iMinLOD <= 1)
    {
        AddBlock(iTX, iTY, iSize, iMinLOD);
        return;
    }

    iTX *= 2;
    iTY *= 2;
    iLev++;

    BuildTree(iTX, iTY, iLev);
    BuildTree(iTX + 1, iTY, iLev);
    BuildTree(iTX, iTY + 1, iLev);
    BuildTree(iTX + 1, iTY + 1, iLev);
}

void Sea::SSE_WaveXZ(SeaVertex** pArray)
{
    glm::vec3 normal[4];
    float nY1[4];
    float nY2[4];
    bool bDistancePass = false;

    for (int i = 0; i < 4; i++)
    {
        int32_t iX11, iX12, iX21, iX22, iY11, iY12, iY21, iY22;

        float distance = Sqr(pArray[i]->position.x - m_CamPos.x) + Sqr(pArray[i]->position.z - m_CamPos.z);
        if (distance > m_MaxSeaDistance * m_MaxSeaDistance)
        {
            normal[i].x = 0.0f;
            normal[i].y = 1.0f;
            normal[i].z = 0.0f;

            nY1[i] = 1.0f;
            nY2[i] = 1.0f;
            pArray[i]->position.y = 0.0f;
            continue;
        }

        bDistancePass = true;

        float x1 = (pArray[i]->position.x + wave1.move.x) * wave1.scale;
        float z1 = (pArray[i]->position.z + wave1.move.z) * wave1.scale;
        iX11 = ffloor(x1 + 0.0f), iX12 = iX11 + 1;
        iY11 = ffloor(z1 + 0.0f), iY12 = iY11 + 1;
        float fX1 = (x1 - iX11);
        float fZ1 = (z1 - iY11);
        iX11 &= (XWIDTH - 1);
        iX12 &= (XWIDTH - 1);
        iY11 &= (XWIDTH - 1);
        iY12 &= (XWIDTH - 1);

        float x2 = (pArray[i]->position.x + wave2.move.x) * wave2.scale;
        float z2 = (pArray[i]->position.z + wave2.move.z) * wave2.scale;
        iX21 = ffloor(x2 + 0.0f), iX22 = iX21 + 1;
        iY21 = ffloor(z2 + 0.0f), iY22 = iY21 + 1;
        float fX2 = (x2 - iX21);
        float fZ2 = (z2 - iY21);
        iX21 &= (XWIDTH - 1);
        iX22 &= (XWIDTH - 1);
        iY21 &= (XWIDTH - 1);
        iY22 &= (XWIDTH - 1);

        float a1, a2, a3, a4;

        a1 = pSeaFrame1[iX11 + iY11 * XWIDTH];
        a2 = pSeaFrame1[iX12 + iY11 * XWIDTH];
        a3 = pSeaFrame1[iX11 + iY12 * XWIDTH];
        a4 = pSeaFrame1[iX12 + iY12 * XWIDTH];
        pArray[i]->position.y = wave1.amplitude * (a1 + fX1 * (a2 - a1) + fZ1 * (a3 - a1) + fX1 * fZ1 * (a4 + a1 - a2 - a3));

        a1 = pSeaFrame2[iX21 + iY21 * XWIDTH];
        a2 = pSeaFrame2[iX22 + iY21 * XWIDTH];
        a3 = pSeaFrame2[iX21 + iY22 * XWIDTH];
        a4 = pSeaFrame2[iX22 + iY22 * XWIDTH];
        pArray[i]->position.y += wave2.amplitude * (a1 + fX2 * (a2 - a1) + fZ2 * (a3 - a1) + fX2 * fZ2 * (a4 + a1 - a2 - a3));

        float nx1, nx2, nx3, nx4, nz1, nz2, nz3, nz4;

        nx1 = pSeaNormalsFrame1[2 * (iX11 + iY11 * XWIDTH) + 0];
        nz1 = pSeaNormalsFrame1[2 * (iX11 + iY11 * XWIDTH) + 1];
        nx2 = pSeaNormalsFrame1[2 * (iX12 + iY11 * XWIDTH) + 0];
        nz2 = pSeaNormalsFrame1[2 * (iX12 + iY11 * XWIDTH) + 1];
        nx3 = pSeaNormalsFrame1[2 * (iX11 + iY12 * XWIDTH) + 0];
        nz3 = pSeaNormalsFrame1[2 * (iX11 + iY12 * XWIDTH) + 1];
        nx4 = pSeaNormalsFrame1[2 * (iX12 + iY12 * XWIDTH) + 0];
        nz4 = pSeaNormalsFrame1[2 * (iX12 + iY12 * XWIDTH) + 1];

        float nX1 = (nx1 + fX1 * (nx2 - nx1) + fZ1 * (nx3 - nx1) + fX1 * fZ1 * (nx4 + nx1 - nx2 - nx3));
        float nZ1 = (nz1 + fX1 * (nz2 - nz1) + fZ1 * (nz3 - nz1) + fX1 * fZ1 * (nz4 + nz1 - nz2 - nz3));

        nx1 = pSeaNormalsFrame2[2 * (iX21 + iY21 * XWIDTH) + 0];
        nz1 = pSeaNormalsFrame2[2 * (iX21 + iY21 * XWIDTH) + 1];
        nx2 = pSeaNormalsFrame2[2 * (iX22 + iY21 * XWIDTH) + 0];
        nz2 = pSeaNormalsFrame2[2 * (iX22 + iY21 * XWIDTH) + 1];
        nx3 = pSeaNormalsFrame2[2 * (iX21 + iY22 * XWIDTH) + 0];
        nz3 = pSeaNormalsFrame2[2 * (iX21 + iY22 * XWIDTH) + 1];
        nx4 = pSeaNormalsFrame2[2 * (iX22 + iY22 * XWIDTH) + 0];
        nz4 = pSeaNormalsFrame2[2 * (iX22 + iY22 * XWIDTH) + 1];

        float nX2 = (nx1 + fX2 * (nx2 - nx1) + fZ2 * (nx3 - nx1) + fX2 * fZ2 * (nx4 + nx1 - nx2 - nx3));
        float nZ2 = (nz1 + fX2 * (nz2 - nz1) + fZ2 * (nz3 - nz1) + fX2 * fZ2 * (nz4 + nz1 - nz2 - nz3));

        nY1[i] = 1.0f - (Sqr(nX1) + Sqr(nZ1));
        nY2[i] = 1.0f - (Sqr(nX2) + Sqr(nZ2));

        normal[i].x = wave1.scale * wave1.amplitude * nX1 + wave2.scale * wave2.amplitude * nX2;
        normal[i].z = wave1.scale * wave1.amplitude * nZ1 + wave2.scale * wave2.amplitude * nZ2;
    }

    if (bDistancePass)
    {
        // INTEL COMMENT:
        // xmm variables.
        __m128 m128X, m128Y, m128Z;
        __m128 m128NX, m128NY, m128NZ;
        __m128 m128Y1, m128Y2;
        __m128 m128SY1, m128SY2;

        // INTEL COMMENT:
        // Gather the Y's.
        SSE_GatherFourFloats(&nY1[0], &nY1[1], &nY1[2], &nY1[3], &m128Y1);
        SSE_GatherFourFloats(&nY2[0], &nY2[1], &nY2[2], &nY2[3], &m128Y2);
        m128SY1 = _mm_sqrt_ps(m128Y1);
        m128SY2 = _mm_sqrt_ps(m128Y2);
        m128Y = _mm_add_ps(m128SY1, m128SY2);

        // INTEL COMMENT:
        // Gather the X's and Z's.
        SSE_GatherFourFloats(&normal[0].x, &normal[1].x, &normal[2].x, &normal[3].x, &m128X);
        SSE_GatherFourFloats(&normal[0].z, &normal[1].z, &normal[2].z, &normal[3].z, &m128Z);

        // INTEL COMMENT:
        // Normalise.
        SSE_Normalise(m128X, m128Y, m128Z, &m128NX, &m128NY, &m128NZ);

        // INTEL COMMENT:
        // Scatter back out.
        SSE_ScatterFourFloats(&pArray[0]->normal.x, &pArray[1]->normal.x, &pArray[2]->normal.x,
            &pArray[3]->normal.x, m128NX);
        SSE_ScatterFourFloats(&pArray[0]->normal.y, &pArray[1]->normal.y, &pArray[2]->normal.y,
            &pArray[3]->normal.y, m128NY);
        SSE_ScatterFourFloats(&pArray[0]->normal.z, &pArray[1]->normal.z, &pArray[2]->normal.z,
            &pArray[3]->normal.z, m128NZ);
    }
    else
    {
        pArray[0]->normal = normal[0];
        pArray[1]->normal = normal[1];
        pArray[2]->normal = normal[2];
        pArray[3]->normal = normal[3];
    }

    pArray[0]->position.y += m_SeaHeightOffset;
    pArray[1]->position.y += m_SeaHeightOffset;
    pArray[2]->position.y += m_SeaHeightOffset;
    pArray[3]->position.y += m_SeaHeightOffset;
}

float Sea::WaveXZ(float x, float z, glm::vec3* pNormal)
{
    int32_t iX11, iX12, iX21, iX22, iY11, iY12, iY21, iY22;

    const float distance = Sqr(x - m_CamPos.x) + Sqr(z - m_CamPos.z);
    if (distance > m_MaxSeaDistance * m_MaxSeaDistance)
    {
        if (pNormal)
            *pNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        return 0.0f;
    }

    const float x1 = (x + wave1.move.x) * wave1.scale;
    const float z1 = (z + wave1.move.z) * wave1.scale;
    iX11 = ffloor(x1 + 0.0f), iX12 = iX11 + 1;
    iY11 = ffloor(z1 + 0.0f), iY12 = iY11 + 1;
    const float fX1 = (x1 - iX11);
    const float fZ1 = (z1 - iY11);
    iX11 &= (XWIDTH - 1);
    iX12 &= (XWIDTH - 1);
    iY11 &= (XWIDTH - 1);
    iY12 &= (XWIDTH - 1);

    const float x2 = (x + wave2.move.x) * wave2.scale;
    const float z2 = (z + wave2.move.z) * wave2.scale;
    iX21 = ffloor(x2 + 0.0f), iX22 = iX21 + 1;
    iY21 = ffloor(z2 + 0.0f), iY22 = iY21 + 1;
    const float fX2 = (x2 - iX21);
    const float fZ2 = (z2 - iY21);
    iX21 &= (XWIDTH - 1);
    iX22 &= (XWIDTH - 1);
    iY21 &= (XWIDTH - 1);
    iY22 &= (XWIDTH - 1);

    float a1, a2, a3, a4;

    a1 = pSeaFrame1[iX11 + iY11 * XWIDTH];
    a2 = pSeaFrame1[iX12 + iY11 * XWIDTH];
    a3 = pSeaFrame1[iX11 + iY12 * XWIDTH];
    a4 = pSeaFrame1[iX12 + iY12 * XWIDTH];
    float fRes = wave1.amplitude * (a1 + fX1 * (a2 - a1) + fZ1 * (a3 - a1) + fX1 * fZ1 * (a4 + a1 - a2 - a3));

    a1 = pSeaFrame2[iX21 + iY21 * XWIDTH];
    a2 = pSeaFrame2[iX22 + iY21 * XWIDTH];
    a3 = pSeaFrame2[iX21 + iY22 * XWIDTH];
    a4 = pSeaFrame2[iX22 + iY22 * XWIDTH];
    fRes += wave2.amplitude * (a1 + fX2 * (a2 - a1) + fZ2 * (a3 - a1) + fX2 * fZ2 * (a4 + a1 - a2 - a3));

    float nx1, nx2, nx3, nx4, nz1, nz2, nz3, nz4;

    nx1 = pSeaNormalsFrame1[2 * (iX11 + iY11 * XWIDTH) + 0];
    nz1 = pSeaNormalsFrame1[2 * (iX11 + iY11 * XWIDTH) + 1];
    nx2 = pSeaNormalsFrame1[2 * (iX12 + iY11 * XWIDTH) + 0];
    nz2 = pSeaNormalsFrame1[2 * (iX12 + iY11 * XWIDTH) + 1];
    nx3 = pSeaNormalsFrame1[2 * (iX11 + iY12 * XWIDTH) + 0];
    nz3 = pSeaNormalsFrame1[2 * (iX11 + iY12 * XWIDTH) + 1];
    nx4 = pSeaNormalsFrame1[2 * (iX12 + iY12 * XWIDTH) + 0];
    nz4 = pSeaNormalsFrame1[2 * (iX12 + iY12 * XWIDTH) + 1];

    const float nX1 = (nx1 + fX1 * (nx2 - nx1) + fZ1 * (nx3 - nx1) + fX1 * fZ1 * (nx4 + nx1 - nx2 - nx3));
    const float nZ1 = (nz1 + fX1 * (nz2 - nz1) + fZ1 * (nz3 - nz1) + fX1 * fZ1 * (nz4 + nz1 - nz2 - nz3));

    nx1 = pSeaNormalsFrame2[2 * (iX21 + iY21 * XWIDTH) + 0];
    nz1 = pSeaNormalsFrame2[2 * (iX21 + iY21 * XWIDTH) + 1];
    nx2 = pSeaNormalsFrame2[2 * (iX22 + iY21 * XWIDTH) + 0];
    nz2 = pSeaNormalsFrame2[2 * (iX22 + iY21 * XWIDTH) + 1];
    nx3 = pSeaNormalsFrame2[2 * (iX21 + iY22 * XWIDTH) + 0];
    nz3 = pSeaNormalsFrame2[2 * (iX21 + iY22 * XWIDTH) + 1];
    nx4 = pSeaNormalsFrame2[2 * (iX22 + iY22 * XWIDTH) + 0];
    nz4 = pSeaNormalsFrame2[2 * (iX22 + iY22 * XWIDTH) + 1];

    const float nX2 = (nx1 + fX2 * (nx2 - nx1) + fZ2 * (nx3 - nx1) + fX2 * fZ2 * (nx4 + nx1 - nx2 - nx3));
    const float nZ2 = (nz1 + fX2 * (nz2 - nz1) + fZ2 * (nz3 - nz1) + fX2 * fZ2 * (nz4 + nz1 - nz2 - nz3));

    if (pNormal)
    {
        const float nY1 = sqrtf(1.0f - (Sqr(nX1) + Sqr(nZ1)));
        const float nY2 = sqrtf(1.0f - (Sqr(nX2) + Sqr(nZ2)));

        glm::vec3 normal;

        normal.x = wave1.scale * wave1.amplitude * nX1 + wave2.scale * wave2.amplitude * nX2;
        normal.z = wave1.scale * wave1.amplitude * nZ1 + wave2.scale * wave2.amplitude * nZ2;
        normal.y = nY1 + nY2;

        *pNormal = glm::normalize(normal);
    }

    fRes += m_SeaHeightOffset;

    return fRes;
}

void Sea::PrepareIndicesForBlock(uint32_t blockIndex)
{
    SeaBlock* pB = &m_Blocks[blockIndex];

    const float fStep = m_GridStep * static_cast<float>(1 << pB->iLOD);
    float fSize = m_GridStep * pB->iSize;
    int32_t x, y, size0 = pB->iSize >> pB->iLOD;

    pB->iIStart = indicesStart;

    const float x1 = static_cast<float>(pB->iTX * pB->iSize) * m_GridStep;
    const float y1 = static_cast<float>(pB->iTY * pB->iSize) * m_GridStep;
    const float x2 = x1 + static_cast<float>(size0) * fStep;
    const float y2 = y1 + static_cast<float>(size0) * fStep;

    pB->iX1 = fftoi(x1 / m_GridStep);
    pB->iX2 = fftoi(x2 / m_GridStep);
    pB->iY1 = fftoi(y1 / m_GridStep);
    pB->iY2 = fftoi(y2 / m_GridStep);

    // analysis
    int32_t i, j;
    for (i = 0; i < static_cast<int32_t>(blockIndex); i++) // if (i == 6)
    {
        SeaBlock* pB2 = &m_Blocks[i];

        // Test Up & Down
        const bool bTestedUp = pB->iY1 == pB2->iY2;
        const bool bTestedDown = pB->iY2 == pB2->iY1;

        // if (!(core.Controls->GetAsyncKeyState('5')<0))
        if (bTestedUp || bTestedDown)
        {
            const int32_t iAddSrc = pB2->iIStart + ((bTestedUp) ? (pB2->iSize0 + 1) * pB2->iSize0 : 0);
            const int32_t iAddDst = pB->iIStart + ((bTestedUp) ? 0 : (pB->iSize0 + 1) * pB->iSize0);

            if ((pB->iX1 >= pB2->iX1 && pB->iX2 <= pB2->iX2) || (pB->iX1 <= pB2->iX1 && pB->iX2 >= pB2->iX2))
            {
                const int32_t iMinX = std::max(pB->iX1, pB2->iX1);
                const int32_t iMaxX = std::min(pB->iX2, pB2->iX2);

                const int32_t iStartDstX = pB->iSize0 * (iMinX - pB->iX1) / (pB->iX2 - pB->iX1);
                const int32_t iStartSrcX = pB2->iSize0 * (iMinX - pB2->iX1) / (pB2->iX2 - pB2->iX1);

                const int32_t iEndDstX = pB->iSize0 * (iMaxX - pB->iX1) / (pB->iX2 - pB->iX1);
                int32_t iEndSrcX = pB2->iSize0 * (iMaxX - pB2->iX1) / (pB2->iX2 - pB2->iX1);

                if (pB->iLOD == pB2->iLOD)
                    for (j = iStartDstX; j <= iEndDstX; j++)
                        pIndices[iAddDst + j] = pIndices[iAddSrc + iStartSrcX + (j - iStartDstX)];
                else
                    for (j = iStartDstX; j <= iEndDstX; j++)
                        pIndices[iAddDst + j] = pIndices[iAddSrc + iStartSrcX + (j - iStartDstX) / 2];

                continue;
            }
        }

        // Test Left & Right
        const bool bTestedLeft = pB->iX1 == pB2->iX2;
        const bool bTestedRight = pB->iX2 == pB2->iX1;
        // if ((core.Controls->GetAsyncKeyState('6')<0))
        if (bTestedLeft || bTestedRight)
        {
            const int32_t iAddSrc = pB2->iIStart + ((bTestedLeft) ? (pB2->iSize0) : 0);
            const int32_t iAddDst = pB->iIStart + ((bTestedLeft) ? 0 : (pB->iSize0));

            if ((pB->iY1 >= pB2->iY1 && pB->iY2 <= pB2->iY2) || (pB->iY1 <= pB2->iY1 && pB->iY2 >= pB2->iY2))
            {
                const int32_t iMinY = std::max(pB->iY1, pB2->iY1);
                const int32_t iMaxY = std::min(pB->iY2, pB2->iY2);

                const int32_t iStartDstY = pB->iSize0 * (iMinY - pB->iY1) / (pB->iY2 - pB->iY1);
                const int32_t iStartSrcY = pB2->iSize0 * (iMinY - pB2->iY1) / (pB2->iY2 - pB2->iY1);

                const int32_t iEndDstY = pB->iSize0 * (iMaxY - pB->iY1) / (pB->iY2 - pB->iY1);
                int32_t iEndSrcY = pB2->iSize0 * (iMaxY - pB2->iY1) / (pB2->iY2 - pB2->iY1);

                if (pB->iLOD == pB2->iLOD)
                    for (j = iStartDstY; j <= iEndDstY; j++)
                        pIndices[iAddDst + j * (pB->iSize0 + 1)] =
                        pIndices[iAddSrc + (iStartSrcY + j - iStartDstY) * (pB2->iSize0 + 1)];
                else
                    for (j = iStartDstY; j <= iEndDstY; j++)
                        pIndices[iAddDst + j * (pB->iSize0 + 1)] =
                        pIndices[iAddSrc + (iStartSrcY + (j - iStartDstY) / 2) * (pB2->iSize0 + 1)];
            }
        }
    }

    int32_t iIFirst = 1000000, iILast = -1000000;

    for (y = 0; y <= size0; y++)
        for (x = 0; x <= size0; x++)
        {
            if (pIndices[indicesStart] != static_cast<uint32_t>(-1))
            {
                indicesStart++;
                continue;
            }

            if (verticesStart < iIFirst)
                iIFirst = verticesStart;
            if (verticesStart > iILast)
                iILast = verticesStart;

            pIndices[indicesStart++] = static_cast<uint32_t>(verticesStart);
            verticesStart++;
        }

    pB->iIFirst = iIFirst;
    pB->iILast = iILast;

    // setup triangles
    int32_t yy, dyy = size0 + 1;
    for (y = 0, yy = 0; y < size0; y++, yy += dyy)
        for (x = 0; x < size0; x++)
        {
            // first triangle
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy + dyy + 1)]);
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy + 1)]);
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy)]);
            // second triangle
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy + dyy)]);
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy + dyy + 1)]);
            *pTriangles++ = static_cast<uint16_t>(pIndices[pB->iIStart + static_cast<uint32_t>(x + yy)]);

            trianglesStart += 2;
        }
}

void Sea::SSE_WaveXZBlock(SeaBlock& pB)
{
    SeaVertex* vTmp[4];
    SeaVertex vFake;

    vFake.position.x = m_CamPos.x + 1e+5f;
    vFake.position.z = m_CamPos.z + 1e+5f;

    float cx, cz, fStep = m_GridStep * static_cast<float>(1 << pB.iLOD);
    float fSize = m_GridStep * pB.iSize;
    int32_t x, y, size0 = pB.iSize >> pB.iLOD;

    float x1 = static_cast<float>(pB.iTX * pB.iSize) * m_GridStep;
    float y1 = static_cast<float>(pB.iTY * pB.iSize) * m_GridStep;
    float x2 = x1 + static_cast<float>(size0) * fStep;
    float y2 = y1 + static_cast<float>(size0) * fStep;

    pB.iX1 = fftoi(x1 / m_GridStep);
    pB.iX2 = fftoi(x2 / m_GridStep);
    pB.iY1 = fftoi(y1 / m_GridStep);
    pB.iY2 = fftoi(y2 / m_GridStep);

    x1 += m_SeaCenterPos.x;
    x2 += m_SeaCenterPos.x;
    y1 += m_SeaCenterPos.z;
    y2 += m_SeaCenterPos.z;

    // CVECTOR normal, vTmp;
    int32_t iCurrentV = 0;

    int32_t iIStart1 = pB.iIStart;
    const int32_t iIFirst = pB.iIFirst;
    const int32_t iILast = pB.iILast;

    // calculate
    for (cz = y1, y = 0; y <= size0; y++, cz += fStep)
    {
        for (cx = x1, x = 0; x <= size0; x++, cx += fStep)
        {
            const uint32_t dwVIndex = pIndices[iIStart1];

            if (static_cast<int32_t>(dwVIndex) < iIFirst || static_cast<int32_t>(dwVIndex) > iILast)
            {
                iIStart1++;
                continue;
            }

            vTmp[iCurrentV] = &pVSea[dwVIndex];
            vTmp[iCurrentV]->position.x = cx;
            vTmp[iCurrentV]->position.z = cz;

            iCurrentV++;

            if (iCurrentV == 4)
            {
                SSE_WaveXZ(&vTmp[0]);
                iCurrentV = 0;

                for (int32_t i = 0; i < 4; i++)
                {
                    vTmp[i]->uv.s = (vTmp[i]->position.x - vTmp[i]->normal.x * 5.3f) * m_BumpScale;
                    vTmp[i]->uv.t = (vTmp[i]->position.z - vTmp[i]->normal.z * 5.3f) * m_BumpScale;
                    vTmp[i]->position.x -= vTmp[i]->normal.x * m_PosShift * 3.0f + m_WorldOffset.x;
                    vTmp[i]->position.z -= vTmp[i]->normal.z * m_PosShift * 3.0f + m_WorldOffset.z;
                }
            }

            /*vTmp.y = WaveXZ(cx, cz, &normal);

            vTmp.x = cx - normal.x * fPosShift * 3.0f;
            vTmp.z = cz - normal.z * fPosShift * 3.0f;

            pVSea[dwVIndex].position = vTmp;
            pVSea[dwVIndex].normal = normal;
            pVSea[dwVIndex].tu = (cx - normal.x * 5.3f) * fBumpScale;
            pVSea[dwVIndex].tv = (cz - normal.z * 5.3f) * fBumpScale;*/

            iIStart1++;
        }
    }

    if (iCurrentV != 0)
    {
        for (int32_t i = iCurrentV; i < 4; i++)
            vTmp[i] = &vFake;

        SSE_WaveXZ(&vTmp[0]);
        for (int32_t i = 0; i < iCurrentV; i++)
        {
            vTmp[i]->uv.s = (vTmp[i]->position.x - vTmp[i]->normal.x * 5.3f) * m_BumpScale;
            vTmp[i]->uv.t = (vTmp[i]->position.z - vTmp[i]->normal.z * 5.3f) * m_BumpScale;

            vTmp[i]->position.x -= vTmp[i]->normal.x * m_PosShift * 3.0f + m_WorldOffset.x;
            vTmp[i]->position.z -= vTmp[i]->normal.z * m_PosShift * 3.0f + m_WorldOffset.z;
        }
    }

    pB.bDone = true;
}

void Sea::CalculateNormalMap(float fFrame, float fAmplitude, float* pfOut, std::vector<uint32_t*>& aFrames)
{
    const int32_t iFrame1 = fftol(fFrame) % aFrames.size();
    const int32_t iFrame2 = (iFrame1 + 1) % aFrames.size();

    const float fDelta = fFrame - iFrame1;

    uint32_t* pB1 = aFrames[iFrame1];
    uint32_t* pB2 = aFrames[iFrame2];

    for (int32_t y = 0; y < YWIDTH; y++)
        for (int32_t x = 0; x < XWIDTH; x++)
        {
            const uint32_t dw1 = pB1[x + y * XWIDTH];
            const uint32_t dw2 = pB2[x + y * XWIDTH];
            const float nx1 = static_cast<float>(static_cast<short>(dw1)) / 32767.5f;
            const float nx2 = static_cast<float>(static_cast<short>(dw2)) / 32767.5f;
            const float nz1 = static_cast<float>(static_cast<short>(dw1 >> 0x10)) / 32767.5f;
            const float nz2 = static_cast<float>(static_cast<short>(dw2 >> 0x10)) / 32767.5f;

            pfOut[2 * (x + y * XWIDTH) + 0] = (nx1 + (nx2 - nx1) * fDelta);
            pfOut[2 * (x + y * XWIDTH) + 1] = (nz1 + (nz2 - nz1) * fDelta);
        }
}

void Sea::CalculateHeightMap(float fFrame, float fAmplitude, float* pfOut, std::vector<uint8_t*>& aFrames)
{
    const int32_t iFrame1 = fftol(fFrame) % aFrames.size();
    const int32_t iFrame2 = (iFrame1 + 1) % aFrames.size();

    const float fDelta = fFrame - iFrame1;

    uint8_t* pB1 = aFrames[iFrame1];
    uint8_t* pB2 = aFrames[iFrame2];

    for (int32_t y = 0; y < YWIDTH; y++)
        for (int32_t x = 0; x < XWIDTH; x++)
        {
            const float f1 = pB1[x + y * XWIDTH];
            const float f2 = pB2[x + y * XWIDTH];

            pfOut[x + y * XWIDTH] = fAmplitude * (f1 + (f2 - f1) * fDelta);
        }
}

void Sea::Realize(float deltaTime)
{
    static float fTmp = 0.0f;

    if (core.Controls->GetDebugAsyncKeyState(VK_SHIFT) < 0 && core.Controls->GetDebugAsyncKeyState('S') < 0)
    {
        if (m_TempFullMode)
        {
            m_TempFullMode = false;
            m_GridStep = m_TempGridStep;
            m_LodScale = m_TempLodScale;
        }
        else
        {
            m_TempFullMode = true;
            m_TempGridStep = m_GridStep;
            m_TempLodScale = m_LodScale;

            m_GridStep = 0.07f;
            m_LodScale = 0.5f;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (m_Stop)
        return;

	wave1.move += deltaTime * wave1.moveSpeed;
    wave2.move += deltaTime * wave2.moveSpeed;

    wave1.frame += deltaTime * wave1.animSpeed;
    wave2.frame += deltaTime * wave2.animSpeed;
    while (wave1.frame >= FRAMES)
        wave1.frame -= FRAMES;
    while (wave2.frame >= FRAMES)
        wave2.frame -= FRAMES;

    m_CamPos = GetCamera()->getPosition();
#ifdef OLD_WORLD_POS
    m_WorldOffset = 0.0f;
#else
    m_WorldOffset = m_CamPos;
    m_WorldOffset.y = 0.f;
#endif

    m_UnderSea = false;

    if (m_CamPos.y < 0.0f)
        m_UnderSea = true;

    if (m_CamPos.y < WaveXZ(m_CamPos.x, m_CamPos.z && m_Started))
        m_UnderSea = true;

    if (!m_UnderSea)
        m_UnderSeaStarted = false;

    if (m_SimpleSea)
    {
        EnvMap_Render2();
        SunRoad_Render2();
    }
    else
    {
        EnvMap_Render();
        SunRoad_Render();
    }

    pFrustumPlanes = rs->GetPlanes();

    float fBlockSize = 256.0f * m_GridStep;
    int32_t iNumBlocks = static_cast<int32_t>(m_MaxDim) / (256 * 2);
    m_SeaCenterPos = glm::vec3(fBlockSize * (static_cast<int32_t>(m_CamPos.x / fBlockSize) - iNumBlocks), m_MaxSeaHeight * 0.5f,
        fBlockSize * (static_cast<int32_t>(m_CamPos.z / fBlockSize) - iNumBlocks));

    verticesStart = 0;
    trianglesStart = 0;
    indicesStart = 0;

    if (!pVolumeTexture && aBumpMaps.size())
    {
        static float fBumpMapFrame = 0.0f;
        fBumpMapFrame += static_cast<float>(deltaTime) * fBumpSpeed * 48.0f;

        uint32_t dw1 = static_cast<int32_t>(fBumpMapFrame) % aBumpMaps.size();
        uint32_t dw2 = static_cast<int32_t>(fBumpMapFrame + 1.0f) % aBumpMaps.size();

        float fAlpha = 255.0f * (fBumpMapFrame - static_cast<float>(static_cast<int32_t>(fBumpMapFrame)));

        RHI::ShaderHandle vertexShader = m_Device->createShaderModule((FilesystemUtilities::GetShadersDir() + "Vulkan/Sea/VK_BumpInterpolate.vert").c_str());
        RHI::ShaderHandle pixelShader = m_Device->createShaderModule((FilesystemUtilities::GetShadersDir() + "Vulkan/Sea/VK_BumpInterpolate.frag").c_str());

        ConstantAlphaBuffer alphaBuffer = { fAlpha };
        RHI::BufferDesc desc{};
        desc.setSize(sizeof(ConstantAlphaBuffer));
        desc.setIsUniformBuffer(true);
        desc.setIsTransferDst(true);
        RHI::BufferHandle bufferHandle = m_Device->createBuffer(desc);

        m_CommandList->beginSingleTimeCommands();
        m_CommandList->writeBuffer(bufferHandle.get(), sizeof(ConstantAlphaBuffer), &alphaBuffer);
        m_CommandList->endSingleTimeCommands();

        RHI::BufferAttachment bufferAttachment = {};
        bufferAttachment
            .setDescriptorInfo(RHI::DescriptorInfo{ RHI::DescriptorType::UNIFORM_BUFFER, RHI::ShaderStageFlagBits::FRAGMENT_BIT })
            .setBuffer(m_ConstantBuffer.get())
            .setSize(sizeof(ConstantAlphaBuffer))
            .setOffset(0);

        RHI::TextureAttachment aBumpAttachment1 = {};
        aBumpAttachment1
            .setDescriptorInfo(RHI::DescriptorInfo{ RHI::DescriptorType::COMBINED_IMAGE_SAMPLER, RHI::ShaderStageFlagBits::FRAGMENT_BIT })
            .setTexture(aBumpMaps[dw1].get())
            .setSampler(m_Sampler.get());

        RHI::TextureAttachment aBumpAttachment2 = {};
        aBumpAttachment2
            .setDescriptorInfo(RHI::DescriptorInfo{ RHI::DescriptorType::COMBINED_IMAGE_SAMPLER, RHI::ShaderStageFlagBits::FRAGMENT_BIT })
            .setTexture(aBumpMaps[dw2].get())
            .setSampler(m_Sampler.get());

        RHI::DescriptorSetInfo dsInfos = { .buffers = {bufferAttachment}, .textures = {aBumpAttachment1, aBumpAttachment2} };

        RHI::BindingLayoutHandle bindingLayout = m_Device->createDescriptorSetLayout(dsInfos);
        RHI::BindingSetHandle bindingSet = m_Device->createDescriptorSet(dsInfos, 1, bindingLayout.get());

        m_CommandList->beginSingleTimeCommands();
        for (uint32_t i = 0; i < MIPSLVLS; i++)
        {
            RHI::FramebufferDesc framebufferDesc{};
            framebufferDesc.addColorAttachment(
                RHI::FramebufferAttachment{
                    pRenderTargetBumpMap.get(),
                    RHI::TextureSubresourse{i , MIPSLVLS} });
            RHI::IRenderPass* renderPass = m_Device->createRenderPass(framebufferDesc);
            RHI::FramebufferHandle framebuffer = m_Device->createFramebuffer(renderPass, framebufferDesc);

            RHI::GraphicsPipelineDesc pipelineDesc{};
            pipelineDesc.VS = vertexShader;
            pipelineDesc.PS = pixelShader;
            pipelineDesc.primType = RHI::PrimitiveType::TriangleList;
            pipelineDesc.renderState.depthStencilState.depthTestEnable = false;

        	RHI::GraphicsPipelineHandle pipelineHandle = m_Device->createGraphicsPipeline(pipelineDesc, framebuffer.get());

            RHI::GraphicsState graphicsState{};
            graphicsState.framebuffer = framebuffer.get();
            graphicsState.bindingSets = { bindingSet };
            graphicsState.pipeline = pipelineHandle.get();

            RHI::DrawArguments drawArgs{};
            drawArgs.startVertexLocation = 0;
            drawArgs.instanceCount = 1;
            drawArgs.vertexCount = 6;
            m_CommandList->draw(drawArgs);

        	//rs->DrawSprites(spr, 1, "bump_interpolate");
        }

        m_CommandList->endSingleTimeCommands();
        m_Device->executeCommandList(m_CommandList.get(), RHI::CommandQueue::Graphics);
    }

    memset(pIndices, 0xFF, NUM_VERTICES * sizeof(pIndices[0]) * 3);

    CalculateHeightMap(wave1.frame, 1.0f / 255.0f, pSeaFrame1, aBumps);
    CalculateNormalMap(wave1.frame, 1.0f / 255.0f, pSeaNormalsFrame1, aNormals);

    CalculateHeightMap(wave2.frame, 1.0f / 255.0f, pSeaFrame2, aBumps);
    CalculateNormalMap(wave2.frame, 1.0f / 255.0f, pSeaNormalsFrame2, aNormals);

    m_Blocks.clear();
    BuildTree(0, 0, 0);
    std::sort(m_Blocks.begin(), m_Blocks.end(), SeaBlock::QSort);

    uint32_t i;
    int32_t iNumVPoints = 0;
    for (i = 0; i < m_Blocks.size(); i++)
    {
        iNumVPoints += m_Blocks[i].iSize0 * m_Blocks[i].iSize0;
        if (iNumVPoints >= NUM_VERTICES)
        {
            m_Blocks.erase(m_Blocks.begin() + i, m_Blocks.end());
            break;
        }
    }

    auto pVSea2 = static_cast<SeaVertex*>(m_Device->mapBufferMemory(m_VerticesSeaBuffer.get(), 0, m_VerticesSeaBuffer->getDesc().size));
    pTriangles = static_cast<uint16_t*>(m_Device->mapBufferMemory(m_IndicesSeaBuffer.get(), 0, m_IndicesSeaBuffer->getDesc().size));

    for (i = 0; i < m_Blocks.size(); i++) {
        PrepareIndicesForBlock(i);
    }

    std::for_each(/*std::execution::par_unseq, */std::begin(m_Blocks), std::end(m_Blocks),
        [this](auto& i) { SSE_WaveXZBlock(i); });

    if (verticesStart && trianglesStart)
        memcpy(pVSea2, pVSea, verticesStart * sizeof(SeaVertex));

    m_Device->unmapBufferMemory(m_VerticesSeaBuffer.get());
    m_Device->unmapBufferMemory(m_IndicesSeaBuffer.get());

    if (verticesStart && trianglesStart)
    {
        glm::mat4 mWorldViewProj;

        glm::mat4 mView = GetCamera()->getViewMatrix();
        glm::mat4 mWorld = glm::identity<glm::mat4>();
        glm::mat4 mProjection = rs->GetProjection();

#ifndef OLD_WORLD_POS
        mView.MoveInversePosition(-m_WorldOffset.x, 0.f, -m_WorldOffset.z);
#endif

        mWorldViewProj = mProjection * mView * mWorld;

        mWorldViewProj = glm::transpose(mWorldViewProj);

        fTmp += deltaTime * fBumpSpeed;
        while (fTmp >= 1.0f)
            fTmp -= 1.0f;

        rs->SetVertexDeclaration(vertexDecl_);

        const auto constant2 = glm::vec4(2.0f, -1.0f, 0.00036621652552071f, (m_FogEnable) ? m_FogSeaDensity : 0.0f);
        const auto shadowConst = glm::vec4(m_FoamV, m_FoamK, m_FoamUV, 6.0f);
        const auto vec4 = glm::vec4(fTmp, fTmp, fTmp, fTmp);
        const auto vec5 = glm::vec4(m_CamPos.x - m_WorldOffset.x, m_CamPos.y, m_CamPos.z - m_WorldOffset.z, 1.0f);
        const auto vec6 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        const auto vec7 = glm::vec4(m_Frenel, 1.0f, 0.5f, 1.0f);
        const auto vec8 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        const auto vec9 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

        ConstantBuffer constantBuffer = {};
        constantBuffer.mvp = mWorldViewProj;
        constantBuffer.constant2 = constant2;
        constantBuffer.shadowConst = shadowConst;
        constantBuffer.animation = vec4;
        constantBuffer.cameraPos = vec5;
        constantBuffer.seaParameters = v4SeaParameters;
        constantBuffer.seaColor = v4SeaColor;
        constantBuffer.skyColor = v4SkyColor;
        constantBuffer.vec6 = vec6;
        constantBuffer.vec7 = vec7;
        constantBuffer.frenelK = vec8;
        constantBuffer.frenelMax = vec9;

        if (m_SimpleSea)
        {
            rs->SetVertexShaderConstantF(GC_FREE + 8, (const float*)&m_TexProjection, 4); // Matrix!!

            rs->SetTexture(0, pVolumeTexture ? pVolumeTexture : pRenderTargetBumpMap);
            rs->SetTexture(1, pReflection);
            rs->SetTexture(2, pVolumeTexture ? pVolumeTexture : pRenderTargetBumpMap);
            rs->SetTexture(3, pReflectionSunroad);

            rs->SetTextureStageState(1, D3DTSS_BUMPENVMAT00, F2DW(0.08f));
            rs->SetTextureStageState(1, D3DTSS_BUMPENVMAT10, F2DW(0.0f));
            rs->SetTextureStageState(1, D3DTSS_BUMPENVMAT01, F2DW(0.0f));
            rs->SetTextureStageState(1, D3DTSS_BUMPENVMAT11, F2DW(0.08f));

            rs->SetTextureStageState(3, D3DTSS_BUMPENVMAT00, F2DW(0.05f));
            rs->SetTextureStageState(3, D3DTSS_BUMPENVMAT10, F2DW(0.0f));
            rs->SetTextureStageState(3, D3DTSS_BUMPENVMAT01, F2DW(0.0f));
            rs->SetTextureStageState(3, D3DTSS_BUMPENVMAT11, F2DW(0.05f));

            rs->DrawIndexedPrimitiveNoVShader(D3DPT_TRIANGLELIST, m_VerticesSeaBuffer, sizeof(SeaVertex), m_IndicesSeaBuffer, 0,
                verticesStart, 0, trianglesStart, "Sea3");

            RHI::DrawArguments drawArgs{};

            m_CommandList->drawIndexed(drawArgs);
        }
        else
        {
            const glm::mat4 mTexProjection = glm::rotate(glm::identity<glm::mat4>(), Math::TWOPI, glm::vec3(0.0f, 0.0f, 1.0f));
            rs->SetVertexShaderConstantF(GC_FREE + 8, (const float*)&mTexProjection, 4); // Matrix!!

            rs->SetTexture(0, pVolumeTexture ? pVolumeTexture : pRenderTargetBumpMap);
            rs->SetTexture(3, pEnvMap);
            rs->DrawIndexedPrimitiveNoVShader(D3DPT_TRIANGLELIST, m_VerticesSeaBuffer, sizeof(SeaVertex), m_IndicesSeaBuffer, 0,
                verticesStart, 0, trianglesStart, "Sea2");

            RHI::DrawArguments drawArgs{};
            drawArgs.vertexCount = NUM_VERTICES;
            m_CommandList->drawIndexed(drawArgs);

            if (m_FoamK > 0.0f && bFoamEnable && bIniFoamEnable)
            {
                // Render sea foam
                const auto vec2 = glm::vec4(m_FoamTextureDisturb, 0.0f, 0.0f, 0.0f);
                rs->SetPixelShaderConstantF(0, (const float*)&vec2, 1);

                rs->TextureSet(0, m_FoamTexture);
                rs->SetTexture(4, pVolumeTexture ? pVolumeTexture : pRenderTargetBumpMap);
                rs->DrawIndexedPrimitiveNoVShader(D3DPT_TRIANGLELIST, m_VerticesSeaBuffer, sizeof(SeaVertex), m_IndicesSeaBuffer, 0,
                    verticesStart, 0, trianglesStart, "Sea2_Foam");

                drawArgs.vertexCount = NUM_VERTICES;
                m_CommandList->drawIndexed(drawArgs);
            }

            rs->SetTexture(0, pVolumeTexture ? pVolumeTexture : pRenderTargetBumpMap);
            rs->SetTexture(3, pSunRoadMap);
            rs->DrawIndexedPrimitiveNoVShader(D3DPT_TRIANGLELIST, m_VerticesSeaBuffer, sizeof(SeaVertex), m_IndicesSeaBuffer, 0,
                verticesStart, 0, trianglesStart, "Sea2_SunRoad");

            drawArgs.vertexCount = NUM_VERTICES;
            m_CommandList->drawIndexed(drawArgs);
        }
    }

    if (m_UnderSea && m_UnderSeaEnable)
    {
        glm::vec3 d(20.0f, 5.0f, 20.0f);
        glm::vec3 d2(54.0f, 10.0f, 54.0f);
        if (!m_UnderSeaStarted)
        {
            // doing a bunch of sea trash for the first time
            aSeaTrash.clear();
            for (int32_t i = 0; i < 1000; i++)
            {
                aSeaTrash.push_back(SeaTrash{});
                // SeaTrash & st = aSeaTrash[aSeaTrash.Add()];
                SeaTrash& st = aSeaTrash.back();
                st.pos = m_CamPos + glm::vec3(RRnd(-d.x, d.x), RRnd(-d.y, d.y), RRnd(-d.z, d.z));
                st.speed = glm::vec3(0.1f, 0.0f, 0.0f);
                st.dwSubTexture = rand() % 4;
                st.size = RRnd(0.01f, 0.05f);
                st.time = static_cast<float>(i) * 0.02f;
            }

            // making a bunch of light poles for the first time
            aSeaLights.clear();
            for (int32_t i = 0; i < 50; i++)
            {
                aSeaLights.push_back(SeaLight{});
                // SeaLight & sl = aSeaLights[aSeaLights.Add()];
                SeaLight& sl = aSeaLights.back();
                sl.pos = m_CamPos + glm::vec3(RRnd(-d2.x, d2.x), RRnd(-d2.y, d2.y), RRnd(-d2.z, d2.z));
                sl.speed = glm::vec3(0.1f, 0.0f, 0.0f);
                sl.dwSubTexture = rand() % 4;
                sl.time = static_cast<float>(i);
            }
        }

        // add 1 item of sea trash every second
        m_LastTrashTime += deltaTime;
        while (m_LastTrashTime >= 0.03f)
        {
            m_LastTrashTime -= 0.03f;

            aSeaTrash.push_back(SeaTrash{});
            // SeaTrash & st = aSeaTrash[aSeaTrash.Add()];
            SeaTrash& st = aSeaTrash.back();
            st.pos = m_CamPos + glm::vec3(RRnd(-d.x, d.x), RRnd(-d.y, d.y), RRnd(-d.z, d.z));
            st.speed = glm::vec3(0.1f, 0.0f, 0.0f);
            st.dwSubTexture = rand() % 4;
            st.time = 20.0f;
            st.size = RRnd(0.01f, 0.05f);
        }

        // Remove the old sea trash
        for (int32_t i = 0; i < aSeaTrash.size(); i++)
        {
            aSeaTrash[i].time -= deltaTime;
            if (aSeaTrash[i].time <= 0.0f)
            {
                // aSeaTrash.ExtractNoShift(i);
                aSeaTrash[i] = aSeaTrash.back();
                aSeaTrash.pop_back();
                i--;
                continue;
            }
            aSeaTrash[i].pos += aSeaTrash[i].speed * deltaTime;
        }
        // Render sea trash
        aTrashRects.clear();
        for (int32_t i = 0; i < aSeaTrash.size(); i++)
        {
            //    RS_RECT & r = aTrashRects[aTrashRects.Add()];
            aTrashRects.push_back(RS_RECT{});
            RS_RECT& r = aTrashRects.back();

            float fAlpha = 1.0f;
            if (aSeaTrash[i].time >= 45.0f)
                fAlpha = 1.0f - (aSeaTrash[i].time - 45.0f) / 5.0f;
            if (aSeaTrash[i].time <= 5.0f)
                fAlpha = 1.0f - (5.0f - aSeaTrash[i].time) / 5.0f;

            r.pos = aSeaTrash[i].pos;
            r.size = aSeaTrash[i].size;
            r.angle = 0.0f;
            r.colorPacked = ARGB(fAlpha * 255.0f, 255, 255, 255);
            r.subTexture = aSeaTrash[i].dwSubTexture;
        }

        if (aTrashRects.size())
        {
            rs->TextureSet(0, m_SeaTrashTexture);
            rs->DrawRects(&aTrashRects[0], aTrashRects.size(), "seatrash", 2, 2);
        }

        // Render schools of fish

        // draw light poles
        // add 1 light pole every second
        m_LastLightTime += deltaTime;
        while (m_LastLightTime >= 1.0f)
        {
            m_LastLightTime -= 1.0f;

            aSeaLights.push_back(SeaLight{});
            // SeaLight & sl = aSeaLights[aSeaLights.Add()];
            SeaLight& sl = aSeaLights.back();
            sl.pos = m_CamPos + glm::vec3(RRnd(-d2.x, d2.x), RRnd(-d2.y, d2.y), RRnd(-d2.z, d2.z));
            sl.speed = glm::vec3(0.1f, 0.0f, 0.0f);
            sl.dwSubTexture = rand() % 4;
            sl.time = 50.0f;
        }

        // Remove the old light poles
        for (int32_t i = 0; i < aSeaLights.size(); i++)
        {
            aSeaLights[i].time -= deltaTime;
            if (aSeaLights[i].time <= 0.0f)
            {
                aSeaLights[i] = aSeaLights.back();
                aSeaLights.pop_back();
                i--;
                continue;
            }
            aSeaLights[i].pos += aSeaLights[i].speed * deltaTime;
        }

        float fAlphaK = (m_CamPos.y < 0.0f) ? std::max(0.0f, 1.0f + m_CamPos.y / 30.0f) : 1.0f;

        // draw light poles
        aLightsRects.clear();
        for (int32_t i = 0; i < aSeaLights.size(); i++)
        {
            aLightsRects.push_back(RS_RECT{});
            // RS_RECT & r = aLightsRects[aLightsRects.Add()];
            RS_RECT& r = aLightsRects.back();
            float fAlpha = 1.0f;
            if (aSeaLights[i].time >= 45.0f)
                fAlpha = 1.0f - (aSeaLights[i].time - 45.0f) / 5.0f;
            if (aSeaLights[i].time <= 5.0f)
                fAlpha = 1.0f - (5.0f - aSeaLights[i].time) / 5.0f;

            glm::vec3 v = aSeaLights[i].pos;
            r.pos = glm::vec3(v.x, WaveXZ(v.x, v.z) - 5.0f, v.z);
            r.size = 20.0f;
            r.angle = 0.0f;
            r.colorPacked = ARGB(0.07f * fAlphaK * fAlpha * 255.0f, 227, 245, 153);
            r.subTexture = aSeaLights[i].dwSubTexture;
        }

        if (aLightsRects.size())
        {
            rs->TextureSet(0, m_SeaLightTexture);
            rs->DrawRects(&aLightsRects[0], aLightsRects.size(), "seatrash", 2, 2, 0.5f);
        }

        m_UnderSeaStarted = true;
    }

    m_Started = true;
}

TestCamera* Sea::GetCamera() const
{
    return;
}
