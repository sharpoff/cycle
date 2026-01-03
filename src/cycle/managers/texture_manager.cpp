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

const TextureID TextureManager::createTexture2D(String filename, String name)
{
    if (auto id = getTextureIDByName(name); id != TextureID::Invalid)
        return id;

    if (filename.empty()) {
        LOGE("%s", "Failed to load texture with empty path!");
        return TextureID::Invalid;
    }

    auto it = filenameTextureMap.find(filename);
    if (it != filenameTextureMap.end())
        return it->second;

    ImageLoadInfo info = {};
    if (!loadImageInfoFromFile(filename, info)) {
        LOGE("Failed to load a texture from path '%s'", filename.c_str());
        return TextureID::Invalid;
    }

    const ImageCreateInfo createInfo = {
        .width = info.width,
        .height = info.height,
        .mipLevels = renderDevice->calculateMipLevels(info.width, info.height),
        .type = VK_IMAGE_VIEW_TYPE_2D,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
    };

    Image image;
    if (!renderDevice->createImage(image, createInfo)) {
        LOGE("Failed to create texture from path '%s'", filename.c_str());
        return TextureID::Invalid;
    }

    renderDevice->uploadImage(image, info);
    freeImageInfo(info);

    if (createInfo.mipLevels > 1) {
        renderDevice->generateMipmaps(image);
    }

    TextureID id = (TextureID)textures.size();
    filenameTextureMap[std::filesystem::absolute(filename)] = id;
    if (!name.empty())
        nameTextureMap[name] = id;

    textures.push_back(image);
    return id;
}

// NOTE: cubemap face textures should have postfixes - back,front,left,right,top,bottom (e.g. my_texture_front.png)
const TextureID TextureManager::createTextureCubeMap(String directory, String name)
{
    if (auto id = getTextureIDByName(name); id != TextureID::Invalid)
        return id;

    const uint8_t faceCount = 6;
    Vector<ImageLoadInfo> infos(faceCount);

    uint8_t loaded = 0;
    for (auto &entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file())
            continue; 

        auto filepath = std::filesystem::path(entry);
        String filestem = filepath.stem();

        bool isLoaded = false;
        if (filestem.ends_with("front")) {
            isLoaded = loadImageInfoFromFile(filepath, infos[0]);
        } else if (filestem.ends_with("back")) {
            isLoaded = loadImageInfoFromFile(filepath, infos[1]);
        } else if (filestem.ends_with("top")) {
            isLoaded = loadImageInfoFromFile(filepath, infos[2]);
        } else if (filestem.ends_with("bottom")) {
            isLoaded = loadImageInfoFromFile(filepath, infos[3]);
        } else if (filestem.ends_with("right")) {
            isLoaded = loadImageInfoFromFile(filepath, infos[4]);
        } else if (filestem.ends_with("left")) {
            isLoaded = loadImageInfoFromFile(filepath, infos[5]);
        }

        if (!isLoaded) {
            LOGE("%s", "Failed to load cubemap texture face!");
            return TextureID::Invalid;
        }

        loaded++;
    }

    assert(loaded == faceCount);

    const ImageCreateInfo createInfo = {
        .width = infos[0].width,
        .height = infos[0].height,
        .arrayLayers = faceCount,
        .mipLevels = renderDevice->calculateMipLevels(infos[0].width, infos[0].height),
        .type = VK_IMAGE_VIEW_TYPE_CUBE,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
    };

    Image image;
    if (!renderDevice->createImage(image, createInfo)) {
        LOGE("Failed to create cubemap texture from directory '%s'", directory.c_str());
        return TextureID::Invalid;
    }

    renderDevice->uploadImageLayers(image, infos);
    freeImageInfos(infos);

    if (createInfo.mipLevels > 1) {
        renderDevice->generateMipmaps(image);
    }

    TextureID id = (TextureID)textures.size();
    filenameTextureMap[std::filesystem::absolute(directory)] = id;
    if (!name.empty())
        nameTextureMap[name] = id;

    textures.push_back(image);
    return id;
}

const TextureID TextureManager::getTextureIDByName(String name)
{
    if (name.empty()) {
        return TextureID::Invalid;
    }

    auto it = nameTextureMap.find(name);
    if (it != nameTextureMap.end())
        return nameTextureMap[name];

    return TextureID::Invalid;
}

bool TextureManager::loadImageInfoFromFile(String filename, ImageLoadInfo &info, bool flip)
{
    info.pixels = stbi_load(filename.c_str(), (int *)&info.width, (int *)&info.height, (int *)&info.channels, STBI_rgb_alpha);
    stbi_set_flip_vertically_on_load(flip);
    if (!info.pixels) {
        freeImageInfo(info);
        return false;
    }

    info.channels = STBI_rgb_alpha;
    info.size = info.width * info.height * info.channels;

    return true;
}

void TextureManager::freeImageInfo(ImageLoadInfo &info)
{
    stbi_image_free(info.pixels);
}

void TextureManager::freeImageInfos(Vector<ImageLoadInfo> &infos)
{
    for (auto &info : infos)
        freeImageInfo(info);
}