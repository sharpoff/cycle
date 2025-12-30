#include "cycle/managers/texture_manager.h"

#include <filesystem>

#include "cycle/logger.h"
#include "cycle/types/id.h"
#include "cycle/globals.h"

#include <stb_image.h>
#include <vulkan/vulkan_core.h>

void TextureManager::init(RenderDevice *renderDevice)
{
    static TextureManager textureManager;
    g_textureManager = &textureManager;

    textureManager.renderDevice = renderDevice;
}

void TextureManager::release()
{
    assert(renderDevice);

    for (auto &texture : textures) {
        renderDevice->destroyImage(texture);
    }
}

const TextureID TextureManager::createTexture(String filename, String name)
{
    assert(renderDevice);

    if (filename.empty()) {
        LOGE("%s", "Failed to load texture with empty path!");
        return TextureID::Invalid;
    }

    auto it = filenameTextureMap.find(filename);
    if (it != filenameTextureMap.end())
        return it->second;

    uint32_t width, height, channels;
    unsigned char *pixels = stbi_load(filename.c_str(), (int *)&width, (int *)&height, (int *)&channels, STBI_rgb_alpha);
    if (!pixels) {
        stbi_image_free(pixels);
        LOGE("Failed to load a texture from path '%s'", filename.c_str());
        return TextureID::Invalid;
    }

    const ImageCreateInfo createInfo = {
        .width = width,
        .height = height,
        .mipLevels = renderDevice->calculateMipLevels(width, height),
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
    };

    Image image;
    if (!renderDevice->createImage(image, createInfo)) {
        LOGE("Failed to create texture from path '%s'", filename.c_str());
        return TextureID::Invalid;
    }

    renderDevice->uploadImageData(image, pixels, width * height * STBI_rgb_alpha);
    stbi_image_free(pixels);

    if (createInfo.mipLevels > 1) {
        renderDevice->generateMipmaps(image);
    }

    TextureID id = (TextureID)textures.size();
    filenameTextureMap[std::filesystem::absolute(filename)] = id;
    textures.push_back(image);
    return id;
}