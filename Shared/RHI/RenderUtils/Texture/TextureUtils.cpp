#include <RHI/RenderUtils/Texture/TextureUtils.hpp>

#include <imgui.h>

#include <stb_image.h>
#include <stb_image_resize.h>

#include "gli/load_ktx.hpp"

namespace RenderUtils
{
    ITexture* createTextureImage(IDevice* device, IRHICommandList* commandList, const char* filename)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels)
        {
            printf("Failed to load [%s] texture\n", filename); fflush(stdout);
            return false;
        }

        TextureDesc desc = {};
        desc.setWidth(texWidth)
            .setHeight(texHeight)
            .setFormat(Format::RGBA8_UNORM);
        ITexture* result = createTextureImageFromData(device, commandList, desc, pixels);

        stbi_image_free(pixels);

        return result;
    }

    ITexture* createTextureImageFromData(IDevice* device, IRHICommandList* commandList, TextureDesc& desc, void* imageData)
    {
        desc.setIsTransferDst(true)
            .setIsShaderResource(true);
        ITexture* tex = device->createImage(desc);

        commandList->updateTextureImage(tex, imageData);

        return tex;
    }

    ITexture* createMIPTextureImage(IDevice* device, IRHICommandList* commandList, const char* filename, uint32_t mipLevels)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels)
        {
            printf("Failed to load [%s] texture\n", filename); fflush(stdout);
            return false;
        }

        uint32_t imgSize = texWidth * texHeight * texChannels;
        uint32_t mipSize = (imgSize * 3) >> 1;
        std::vector<uint8_t> mipData(mipSize);

        uint8_t* dst = mipData.data();
        uint8_t* src = dst;
        memcpy(dst, pixels, imgSize);

        uint32_t w = texWidth, h = texHeight;
        for (uint32_t i = 1; i < mipLevels; i++)
        {
            dst += (w * h * texChannels) >> 2;

            stbir_resize_uint8(src, w, h, 0,
                dst, w / 2, h / 2, 0, texChannels);

            w >>= 1;
            h >>= 1;
            src = dst;
        }

        TextureDesc desc = {};
        desc.setWidth(texWidth)
            .setHeight(texHeight)
            .setMipLevels(mipLevels)
            .setFormat(Format::RGBA8_UNORM);
        ITexture* result = createMIPTextureImageFromData(device, commandList, desc, mipData.data());

        stbi_image_free(pixels);

        return result;
    }

    ITexture* createMIPTextureImageFromData(IDevice* device, IRHICommandList* commandList, TextureDesc& desc, void* mipData)
    {
        desc.setIsTransferDst(true)
            .setIsShaderResource(true);
        ITexture* tex = device->createImage(desc);

        // now allocate staging buffer for all MIP levels
        uint32_t bytesPerPixel = bytesPerTexFormat(convertFormat(tex->getDesc().format));

        size_t layerSize = tex->getDesc().width * tex->getDesc().height * bytesPerPixel;
        size_t imageSize = layerSize * tex->getDesc().layerCount;

        uint32_t w = tex->getDesc().width, h = tex->getDesc().height;
        for (uint32_t i = 1; i < tex->getDesc().mipLevels; i++)
        {
            w >>= 1;
            h >>= 1;
            imageSize += w * h * bytesPerPixel * tex->getDesc().layerCount;
        }

        BufferDesc stagingDesc = BufferDesc{}
            .setSize(imageSize)
            .setIsTransferSrc(true)
            .setMemoryProperties(MemoryPropertiesBits::HOST_VISIBLE_BIT | MemoryPropertiesBits::HOST_COHERENT_BIT);
        IBuffer* stagingBuffer = dynamic_cast<IBuffer*>(createBuffer(stagingDesc));

        uploadBufferData(stagingBuffer->memory, 0, mipData, imageSize);
        
        commandList->transitionImageLayout(tex, ImageLayout::UNDEFINED, ImageLayout::TRANSFER_DST_OPTIMAL);
        // TODO:fix command list ussie
        commandList->copyMIPBufferToImage(stagingBuffer, tex, bytesPerPixel);
        commandList->transitionImageLayout(tex, ImageLayout::TRANSFER_DST_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

        delete stagingBuffer;

        return tex;
    }

    ITexture* addRGBATexture(IDevice* device, IRHICommandList* commandList, TextureDesc& desc, void* data)
    {
        desc.setFormat(Format::RGBA8_UNORM);
        ITexture* tex = dynamic_cast<ITexture*>(createTextureImageFromData(device, commandList, desc, data));

        if (!tex)
        {
            printf("Cannot create solid texture\n");
            exit(EXIT_FAILURE);
        }

        commandList->transitionImageLayout(tex, ImageLayout::UNDEFINED, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

        if (!device->createImageView(tex, ImageAspectFlagBits::COLOR_BIT))
        {
            printf("Cannot create image view for 2d texture\n");
            exit(EXIT_FAILURE);
        }

        ISampler* sampler = device->createTextureSampler();
        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(tex);
        return tex;
    }

    ITexture* addSolidRGBATexture(IDevice* device, IRHICommandList* commandList, uint32_t color)
    {
        TextureDesc desc = {};
        desc.setWidth(1)
            .setHeight(1)
            .setDepth(1)
            .setFormat(Format::RGBA8_UNORM);
        ITexture* tex = dynamic_cast<ITexture*>(createTextureImageFromData(device, commandList, desc, &color));

        if (!tex)
        {
            printf("Cannot create solid texture\n");
            exit(EXIT_FAILURE);
        }

        commandList->transitionImageLayout(tex, ImageLayout::UNDEFINED, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

        if (!device->createImageView(tex, ImageAspectFlagBits::COLOR_BIT))
        {
            printf("Cannot create image view for solid texture\n");
            exit(EXIT_FAILURE);
        }

        ISampler* sampler = device->createTextureSampler();
        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(tex);

        return tex;
    }

    ITexture* loadCubemap(IDevice* device, IRHICommandList* commandList, const char* fileName, uint32_t mipLevels)
    {
        ITexture* tex = nullptr;

        uint32_t w = 0, h = 0;

        if (mipLevels > 1)
            tex = createMIPCubeTextureImage(fileName, mipLevels, &w, &h);
        else
            tex = createCubeTextureImage(fileName, &w, &h);

        device->createImageView(tex, ImageAspectFlagBits::COLOR_BIT);
        ISampler* sampler = device->createTextureSampler();

        TextureDesc desc = tex->getDesc();
        desc.setWidth(w);
        desc.setHeight(h);
        ITexture* texture = dynamic_cast<ITexture*>(tex);
        texture->desc = desc;

        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(cubemap);

        return tex;
    }

    ITexture* loadKTX(IDevice* device, IRHICommandList* commandList, const char* fileName)
    {
        gli::texture gliTex = gli::load_ktx(fileName);
        gli::tvec3<uint32_t> extent(gliTex.extent(0));

        TextureDesc desc = {};
        desc.setWidth(extent.x)
            .setHeight(extent.y)
            .setWidth(4)
            .setFormat(Format::RG16_FLOAT);

        ITexture* ktx = createTextureImageFromData(device, commandList, desc,(uint8_t*)gliTex.data(0, 0, 0));

        if (!ktx)
        {
            printf("ModelRenderer: failed to load BRDF LUT texture \n");
            exit(EXIT_FAILURE);
        }

        device->createImageView(ktx, ImageAspectFlagBits::COLOR_BIT);

        SamplerDesc samplerDesc = {};
        samplerDesc.setAddressAll(SamplerAddressMode::CLAMP_TO_EDGE);
        ISampler* sampler = device->createTextureSampler(samplerDesc);

        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(ktx);

        return ktx;
    }

    ITexture* loadTexture2D(IDevice* device, IRHICommandList* commandList, const char* fileName)
    {
        ITexture* tex = createTextureImage(device, commandList, fileName);
        if (!tex)
        {
            printf("Cannot load %s 2D texture file\n", fileName);
            exit(EXIT_FAILURE);
        }

        commandList->transitionImageLayout(tex, ImageLayout::UNDEFINED, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

        if (!device->createImageView(tex, ImageAspectFlagBits::COLOR_BIT))
        {
            printf("Cannot create image view for 2d texture (%s)\n", fileName);
            exit(EXIT_FAILURE);
        }

        ISampler* sampler = device->createTextureSampler();
        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(tex);
        return tex;
    }

    ITexture* createFontTexture(IDevice* device, IRHICommandList* commandList, const char* fontFile)
    {
        // TODO: fix loading resources
        ImGuiIO& io = ImGui::GetIO();

        // Build texture atlas
        ImFontConfig cfg = ImFontConfig();
        cfg.FontDataOwnedByAtlas = false;
        cfg.RasterizerMultiply = 1.5f;
        cfg.SizePixels = 768.0f / 32.0f;
        cfg.PixelSnapH = true;
        cfg.OversampleH = 4;
        cfg.OversampleV = 4;
        ImFont* Font = io.Fonts->AddFontFromFileTTF(fontFile, cfg.SizePixels, &cfg);


        unsigned char* pixels = nullptr;
        int texWidth = 1, texHeight = 1;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);

        if (!pixels)
        {
            printf("Failed to load texture\n"); fflush(stdout);
            return nullptr;
        }

        TextureDesc desc = {};
        desc.setWidth(texWidth)
            .setHeight(texHeight)
            .setFormat(Format::RGBA8_UNORM);
        ITexture* tex = createTextureImageFromData(device, commandList, desc, pixels);
        if (!tex)
        {
            printf("Failed to create texture\n"); fflush(stdout);
            return nullptr;
        }

        device->createImageView(tex, ImageAspectFlagBits::COLOR_BIT);
        ISampler* sampler = device->createTextureSampler();

        /* This is not strictly necessary, a font can be any texture */
        io.Fonts->TexID = (ImTextureID)0;
        io.FontDefault = Font;
        io.DisplayFramebufferScale = ImVec2(1, 1);

        //m_Resources.allTextures.push_back(res);
        //return res;
        return tex;
    }
}
