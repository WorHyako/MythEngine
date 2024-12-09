#include <RHI/RHIVulkan/VulkanBackend.hpp>

#include <UtilsCubemap.hpp>
#include <imgui.h>

#include "stb_image.h"
//#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

#include <gli/load_ktx.hpp>

namespace RHI::Vulkan
{
    static void float24to32(int w, int h, const float* img24, float* img32)
    {
        const int numPixels = w * h;
        for (int i = 0; i != numPixels; i++)
        {
            *img32++ = *img24++;
            *img32++ = *img24++;
            *img32++ = *img24++;
            *img32++ = 1.0f;
        }
    }

    static VkImageCreateInfo fillImageInfo()
    {
	    
    }

    VkFormat Device::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_Context.physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        printf("failed to find supported format!\n");
        exit(0);
    }

    uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_Context.physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        return 0xFFFFFFFF;
    }

    VkFormat Device::findDepthFormat()
    {
        return findSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    ITexture* Device::addRGBATexture(int texWidth, int texHeight, void* data)
    {
        Texture* tex = new Texture();
        tex->width = texWidth;
        tex->height = texHeight;
        tex->depth = 1;
        tex->format = VK_FORMAT_R8G8B8A8_UNORM;
        if (!createTextureImageFromData(tex->image.image, tex->image.imageMemory,
            data, texWidth, texHeight, tex->format))
        {
            printf("Cannot create solid texture\n");
            exit(EXIT_FAILURE);
        }

        // TODO:fix command list ussie
        //transitionImageLayout(tex->image.image, tex->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if (!createImageView(tex->image.image, tex->format, VK_IMAGE_ASPECT_COLOR_BIT, &tex->image.imageView))
        {
            printf("Cannot create image view for 2d texture\n");
            exit(EXIT_FAILURE);
        }

        createTextureSampler(&tex->sampler);
        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(tex);
        return tex;
    }

    ITexture* Device::addSolidRGBATexture(uint32_t color)
    {
        Texture* tex = new Texture();
        tex->width = 1;
        tex->height = 1;
        tex->depth = 1;
        tex->format = VK_FORMAT_R8G8B8A8_UNORM;
        if (!createTextureImageFromData(tex->image.image, tex->image.imageMemory,
            &color, 1, 1, tex->format))
        {
            printf("Cannot create solid texture\n");
            exit(EXIT_FAILURE);
        }

        // TODO:fix command list ussie
        //transitionImageLayout(tex->image.image, tex->format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if (!createImageView(tex->image.image, tex->format, VK_IMAGE_ASPECT_COLOR_BIT, &tex->image.imageView))
        {
            printf("Cannot create image view for solid texture\n");
            exit(EXIT_FAILURE);
        }

        createTextureSampler(&tex->sampler);
        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(tex);

        return tex;
    }

    ITexture* Device::addColorTexture(int texWidth, int texHeight, VkFormat colorFormat, VkFilter minFilter, VkFilter maxFilter, VkSamplerAddressMode addressMode)
    {
        const uint32_t w = (texWidth > 0) ? texWidth : m_DeviceDesc.framebufferWidth;
        const uint32_t h = (texHeight > 0) ? texHeight : m_DeviceDesc.framebufferHeight;

        Texture* tex = new Texture();
        tex->width = w;
        tex->height = h;
        tex->depth = 1;
        tex->format = colorFormat;

        if (!createOffscreenImage(
            tex->image.image, tex->image.imageMemory,
            w, h, colorFormat, 1, 0))
        {
            printf("Cannot create color texture\n");
            exit(EXIT_FAILURE);
        }

        createImageView(tex->image.image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, &tex->image.imageView);
        createTextureSampler(&tex->sampler, minFilter, maxFilter, addressMode);

        // TODO:fix command list ussie
        //transitionImageLayout(tex->image.image, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(tex);

        return tex;
    }

    ITexture* Device::addDepthTexture(int texWidth, int texHeight, VkImageLayout layout)
    {
        const uint32_t w = (texWidth > 0) ? texWidth : m_DeviceDesc.framebufferWidth;
        const uint32_t h = (texHeight > 0) ? texHeight : m_DeviceDesc.framebufferHeight;

        const VkFormat depthFormat = findDepthFormat();

        Texture* tex = new Texture();
        tex->width = w;
        tex->height = h;
        tex->depth = 1;
        tex->format = depthFormat;

        if (!createImage(w, h, depthFormat,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, tex->image.image, tex->image.imageMemory))
        {
            printf("Cannot create depth texture\n");
            exit(EXIT_FAILURE);
        }

        createImageView(tex->image.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &tex->image.imageView);
        // TODO:fix command list ussie
        //transitionImageLayout(tex->image.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, layout/*VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL*/);

        if (!createDepthSampler(&tex->sampler))
        {
            printf("Cannot create a depth sampler");
            exit(EXIT_FAILURE);
        }

        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(tex);

        return tex;
    }

    bool Device::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkImageCreateFlags flags, uint32_t mipLevels)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = nullptr;
        imageInfo.flags = flags;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
        imageInfo.extent = VkExtent3D{ width, height, 1 };
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = (uint32_t)((flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1);
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = tiling;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.queueFamilyIndexCount = 0;
        imageInfo.pQueueFamilyIndices = nullptr;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VK_CHECK(vkCreateImage(m_Context.device, &imageInfo, nullptr, &image));

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_Context.device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        VK_CHECK(vkAllocateMemory(m_Context.device, &allocInfo, nullptr, &imageMemory));

        vkBindImageMemory(m_Context.device, image, imageMemory, 0);
        return true;
    }

    bool Device::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageView* imageView, VkImageViewType viewType, uint32_t layerCount, uint32_t mipLevels)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;
        viewInfo.flags = 0;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = layerCount;

        return (vkCreateImageView(m_Context.device, &viewInfo, nullptr, imageView) == VK_SUCCESS);
    }

    bool Device::createTextureSampler(VkSampler* sampler, VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.pNext = nullptr;
        samplerInfo.flags = 0;
        samplerInfo.magFilter = magFilter;
        samplerInfo.minFilter = minFilter;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = addressMode; // VK_SAMPLER_ADDRESS_MODE_REPEAT,
        samplerInfo.addressModeV = addressMode; // VK_SAMPLER_ADDRESS_MODE_REPEAT,
        samplerInfo.addressModeW = addressMode; // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE VK_SAMPLER_ADDRESS_MODE_REPEAT,
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        return (vkCreateSampler(m_Context.device, &samplerInfo, nullptr, sampler) == VK_SUCCESS);
    }

    bool Device::createDepthSampler(VkSampler* sampler)
    {
        VkSamplerCreateInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        si.pNext = nullptr;
        si.magFilter = VK_FILTER_LINEAR;
        si.minFilter = VK_FILTER_LINEAR;
        si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.mipLodBias = 0.0f;
        si.maxAnisotropy = 1.0f;
        si.minLod = 0.0f;
        si.maxLod = 1.0f;
        si.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        return (vkCreateSampler(m_Context.device, &si, nullptr, sampler) == VK_SUCCESS);
    }

    /** Offscreen rendering helpers */
    bool Device::createOffscreenImage(
        VkImage& textureImage, VkDeviceMemory& textureImageMemory,
        uint32_t texWidth, uint32_t texHeight,
        VkFormat texFormat,
        uint32_t layerCount, VkImageCreateFlags flags)
    {
        return createImage(texWidth, texHeight, texFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT /* necessary only for screenshot */ | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory, flags);
    }

    void Device::destroyVulkanTexture(Texture& texture)
    {
        destroyVulkanImage(texture.image);
        vkDestroySampler(m_Context.device, texture.sampler, nullptr);
    }

    void Device::destroyVulkanImage(VulkanImage& image)
    {
        vkDestroyImageView(m_Context.device, image.imageView, nullptr);
        vkDestroyImage(m_Context.device, image.image, nullptr);
        vkFreeMemory(m_Context.device, image.imageMemory, nullptr);
    }

    bool Device::createTextureImage(const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* outTexWidth, uint32_t* outTexHeight)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (!pixels)
        {
            printf("Failed to load [%s] texture\n", filename); fflush(stdout);
            return false;
        }

        bool result = createTextureImageFromData(textureImage, textureImageMemory, pixels, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM);

        stbi_image_free(pixels);

        if (outTexWidth && outTexHeight)
        {
            *outTexWidth = (uint32_t)texWidth;
            *outTexHeight = (uint32_t)texHeight;
        }

        return result;
    }

    bool Device::createMIPTextureImage(const char* filename, uint32_t mipLevels, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width, uint32_t* height)
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

        bool result = createMIPTextureImageFromData(textureImage, textureImageMemory,
            mipData.data(), mipLevels, texWidth, texHeight,
            VK_FORMAT_R8G8B8A8_UNORM);

        stbi_image_free(pixels);

        if (width && height)
        {
            *width = texWidth;
            *height = texHeight;
        }

        return true;
    }

    bool Device::createCubeTextureImage(const char* filename, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width, uint32_t* height)
    {
        int w, h, comp;
        const float* img = stbi_loadf(filename, &w, &h, &comp, 3);
        std::vector<float> img32(w * h * 4);

        float24to32(w, h, img, img32.data());

        if (!img)
        {
            printf("Failed to load [%s] texture\n", filename); fflush(stdout);
            return false;
        }

        stbi_image_free((void*)img);

        Bitmap in(w, h, 4, eBitmapFormat_Float, img32.data());
        Bitmap out = convertEquirectangularMapToVerticalCross(in);

        Bitmap cube = convertVerticalCrossToCubeMapFaces(out);

        if (width && height)
        {
            *width = w;
            *height = h;
        }

        return createTextureImageFromData(textureImage, textureImageMemory,
            cube.data_.data(), cube.w_, cube.h_,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
    }

    bool Device::createMIPCubeTextureImage(const char* filename, uint32_t mipLevels, VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t* width, uint32_t* height)
    {
        int comp;
        int texWidth, texHeight;
        const float* img = stbi_loadf(filename, &texWidth, &texHeight, &comp, 3);

        if (!img) {
            printf("Failed to load [%s] texture\n", filename); fflush(stdout);
            return false;
        }

        uint32_t imageSize = texWidth * texHeight * 4;
        uint32_t mipSize = imageSize * 6;

        uint32_t w = texWidth, h = texHeight;
        for (uint32_t i = 1; i < mipLevels; i++)
        {
            imageSize = w * h * 4;
            w >>= 1;
            h >>= 1;
            mipSize += imageSize;
        }

        std::vector<float> mipData(mipSize);
        float* src = mipData.data();
        float* dst = mipData.data();

        w = texWidth;
        h = texHeight;
        float24to32(w, h, img, dst);

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            imageSize = w * h * 4;
            dst += w * h * 4;
            stbir_resize_float_generic(
                src, w, h, 0, dst, w / 2, h / 2, 0, 4,
                STBIR_ALPHA_CHANNEL_NONE, 0, STBIR_EDGE_CLAMP, STBIR_FILTER_CUBICBSPLINE, STBIR_COLORSPACE_LINEAR, nullptr);

            w >>= 1;
            h >>= 1;
            src = dst;
        }

        src = mipData.data();
        dst = mipData.data();

        std::vector<float> mipCube(mipSize * 6);
        float* mip = mipCube.data();

        w = texWidth;
        h = texHeight;
        uint32_t faceSize = w / 4;
        for (uint32_t i = 0; i < mipLevels; i++)
        {
            Bitmap in(w, h, 4, eBitmapFormat_Float, src);
            Bitmap out = convertEquirectangularMapToVerticalCross(in);
            Bitmap cube = convertVerticalCrossToCubeMapFaces(out);

            imageSize = faceSize * faceSize * 4;

            memcpy(mip, cube.data_.data(), 6 * imageSize * sizeof(float));
            mip += imageSize * 6;

            src += w * h * 4;
            w >>= 1;
            h >>= 1;
        }

        stbi_image_free((void*)img);

        if (width && height)
        {
            *width = texWidth;
            *height = texHeight;
        }

        return createMIPTextureImageFromData(
            textureImage, textureImageMemory,
            mipCube.data(), mipLevels, faceSize, faceSize,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
    }

    bool Device::createTextureImageFromData(
        VkImage& textureImage, VkDeviceMemory& textureImageMemory,
        void* imageData, uint32_t texWidth, uint32_t texHeight,
        VkFormat texFormat,
        uint32_t layerCount, VkImageCreateFlags flags)
    {
        createImage(texWidth, texHeight, texFormat, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage, textureImageMemory, flags);

        return updateTextureImage(textureImage, textureImageMemory, texWidth, texHeight, texFormat, layerCount, imageData);
    }

    bool Device::createMIPTextureImageFromData(
        VkImage& textureImage, VkDeviceMemory& textureImageMemory,
        void* mipData, uint32_t mipLevels, uint32_t texWidth, uint32_t texHeight,
        VkFormat texFormat,
        uint32_t layerCount, VkImageCreateFlags flags)
    {
        createImage(
            texWidth, texHeight, texFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage, textureImageMemory, flags, mipLevels);

        // now allocate staging buffer for all MIP levels
        uint32_t bytesPerPixel = bytesPerTexFormat(texFormat);

        VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
        VkDeviceSize imageSize = layerSize * layerCount;

        uint32_t w = texWidth, h = texHeight;
        for (uint32_t i = 1; i < mipLevels; i++)
        {
            w >>= 1;
            h >>= 1;
            imageSize += w * h * bytesPerPixel * layerCount;
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        uploadBufferData(stagingBufferMemory, 0, mipData, imageSize);

        // TODO:fix command list ussie
        //transitionImageLayout(textureImage, texFormat, VK_IMAGE_LAYOUT_UNDEFINED/*sourceImageLayout*/, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount, mipLevels);
        // TODO:fix command list ussie
        //copyMIPBufferToImage(stagingBuffer, textureImage, mipLevels, texWidth, texHeight, bytesPerPixel, layerCount);
        // TODO:fix command list ussie
        //transitionImageLayout(textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount, mipLevels);

        vkDestroyBuffer(m_Context.device, stagingBuffer, nullptr);
        vkFreeMemory(m_Context.device, stagingBufferMemory, nullptr);

        return true;
    }

    bool Device::updateTextureImage(VkImage& textureImage, VkDeviceMemory& textureImageMemory, uint32_t texWidth, uint32_t texHeight, VkFormat texFormat, uint32_t layerCount, const void* imageData, VkImageLayout sourceImageLayout)
    {
        uint32_t bytesPerPixel = bytesPerTexFormat(texFormat);

        VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
        VkDeviceSize imageSize = layerSize * layerCount;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        uploadBufferData(stagingBufferMemory, 0, imageData, imageSize);

        // TODO:fix command list ussie
        //transitionImageLayout(textureImage, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount);
        // TODO:fix command list ussie
        //copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount);
        // TODO:fix command list ussie
        //transitionImageLayout(textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount);

        vkDestroyBuffer(m_Context.device, stagingBuffer, nullptr);
        vkFreeMemory(m_Context.device, stagingBufferMemory, nullptr);

        return true;
    }


    Texture Device::loadCubemap(const char* fileName, uint32_t mipLevels)
    {
        Texture cubemap;

        uint32_t w = 0, h = 0;

        if (mipLevels > 1)
            createMIPCubeTextureImage(fileName, mipLevels, cubemap.image.image, cubemap.image.imageMemory, &w, &h);
        else
            createCubeTextureImage(fileName, cubemap.image.image, cubemap.image.imageMemory, &w, &h);

        createImageView(cubemap.image.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &cubemap.image.imageView, VK_IMAGE_VIEW_TYPE_CUBE, 6, mipLevels);
        createTextureSampler(&cubemap.sampler);

        cubemap.format = VK_FORMAT_R32G32B32A32_SFLOAT;

        cubemap.width = w;
        cubemap.height = h;
        cubemap.depth = 1;

        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(cubemap);

        // TODO: fix loading resources
        //return cubemap;
        return Texture();
    }

    Texture Device::loadKTX(const char* fileName)
    {
        gli::texture gliTex = gli::load_ktx(fileName);
        gli::tvec3<uint32_t> extent(gliTex.extent(0));

        Texture ktx{};
        ktx.width = extent.x;
        ktx.height = extent.y;
        ktx.depth = 4;

        if (!createTextureImageFromData(ktx.image.image, ktx.image.imageMemory,
            (uint8_t*)gliTex.data(0, 0, 0), ktx.width, ktx.height, VK_FORMAT_R16G16_SFLOAT))
        {
            printf("ModelRenderer: failed to load BRDF LUT texture \n");
            exit(EXIT_FAILURE);
        }

        createImageView(ktx.image.image, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &ktx.image.imageView);
        createTextureSampler(&ktx.sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(ktx);

        // TODO: fix loading resources
        //return ktx;
        return Texture();
    }

    Texture Device::loadTexture2D(const char* fileName)
    {
        Texture tex;
        if (!createTextureImage(fileName, tex.image.image, tex.image.imageMemory, &tex.width, &tex.height))
        {
            printf("Cannot load %s 2D texture file\n", fileName);
            exit(EXIT_FAILURE);
        }

        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        // TODO:fix command list ussie
        //transitionImageLayout(tex.image.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if (!createImageView(tex.image.image, format, VK_IMAGE_ASPECT_COLOR_BIT, &tex.image.imageView))
        {
            printf("Cannot create image view for 2d texture (%s)\n", fileName);
            exit(EXIT_FAILURE);
        }

        createTextureSampler(&tex.sampler);
        // TODO:fix allocation issue
        //m_Resources.allTextures.push_back(tex);
        // TODO: fix loading resources
        //return tex;
        return Texture();
    }


    Texture Device::createFontTexture(const char* fontFile)
    {
        // TODO: fix loading resources
        //ImGuiIO& io = ImGui::GetIO();
        //VulkanImage img{};
        //img.image = VK_NULL_HANDLE;
        //Texture res{};
        //res.image = img;

        //// Build texture atlas
        //ImFontConfig cfg = ImFontConfig();
        //cfg.FontDataOwnedByAtlas = false;
        //cfg.RasterizerMultiply = 1.5f;
        //cfg.SizePixels = 768.0f / 32.0f;
        //cfg.PixelSnapH = true;
        //cfg.OversampleH = 4;
        //cfg.OversampleV = 4;
        //ImFont* Font = io.Fonts->AddFontFromFileTTF(fontFile, cfg.SizePixels, &cfg);


        //unsigned char* pixels = nullptr;
        //int texWidth = 1, texHeight = 1;
        //io.Fonts->GetTexDataAsRGBA32(&pixels, &texWidth, &texHeight);

        //if (!pixels || !createTextureImageFromData(res.image.image, res.image.imageMemory, pixels, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM))
        //{
        //    printf("Failed to load texture\n"); fflush(stdout);
        //    return res;
        //}

        //createImageView(res.image.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &res.image.imageView);
        //createTextureSampler(&res.sampler);

        ///* This is not strictly necessary, a font can be any texture */
        //io.Fonts->TexID = (ImTextureID)0;
        //io.FontDefault = Font;
        //io.DisplayFramebufferScale = ImVec2(1, 1);

        //m_Resources.allTextures.push_back(res);
        //return res;
        return Texture();
    }
}
