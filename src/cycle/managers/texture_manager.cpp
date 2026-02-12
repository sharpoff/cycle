#include "cycle/managers/texture_manager.h"

#include <filesystem>
#include <assert.h>

#include "cycle/logger.h"
#include "cycle/types/id.h"

#include "ktx.h"
#include "stb_image.h"

TextureManager *g_textureManager = nullptr;

void TextureManager::init(RenderDevice *renderDevice)
{
    static TextureManager instance;
    g_textureManager = &instance;

    instance.renderDevice = renderDevice;
}

TextureManager *TextureManager::get()
{
    return g_textureManager;
}

void TextureManager::release()
{
    assert(renderDevice);

    for (auto &texture : textures) {
        renderDevice->destroyImage(texture);
    }
}

const TextureID TextureManager::createTexture(std::filesystem::path filepath, VkFormat format, String name)
{
    if (auto id = getTextureIDByName(name); id != TextureID::Invalid)
        return id;

    filepath = std::filesystem::absolute(filepath);

    if (filepath.empty()) {
        LOGE("%s", "Failed to load texture with empty path!");
        return TextureID::Invalid;
    }

    auto it = filenameTextureMap.find(filepath);
    if (it != filenameTextureMap.end())
        return it->second;

    // check if compressed ktx image exists
    std::filesystem::path file = filepath;
    file.replace_extension(".ktx");
    std::filesystem::path ktxPath = file.parent_path() / "compressed" / file.filename();

    if (std::filesystem::exists(ktxPath))
        filepath = ktxPath;

    ImageLoadInfo info = {};
    if (!loadImageInfo(filepath, info)) {
        LOGE("Failed to load a texture from path '%s'", filepath.c_str());
        return TextureID::Invalid;
    }

    const ImageCreateInfo createInfo = {
        .width = info.width,
        .height = info.height,
        .arrayLayers = info.arrayLayers,
        .mipLevels = info.mipLevels,
        .type = info.arrayLayers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .format = format,
    };

    Image image = renderDevice->createImage(createInfo);
    renderDevice->uploadImage(image, info);
    freeImageInfo(info);

    // generate mipmaps for non ktx images
    if (!info.textureKTX && createInfo.mipLevels > 1)
        renderDevice->generateMipmaps(image);

    TextureID id = (TextureID)textures.size();
    filenameTextureMap[std::filesystem::absolute(filepath)] = id;
    if (!name.empty())
        nameTextureMap[name] = id;

    textures.push_back(image);
    return id;
}

const TextureID TextureManager::createTextureFromMem(unsigned char *data, uint32_t size, VkFormat format, String name)
{
    if (auto id = getTextureIDByName(name); id != TextureID::Invalid)
        return id;

    ImageLoadInfo info = {};
    if (!loadImageInfo(data, size, info)) {
        LOGE("%s", "Failed to load a texture from memory");
        return TextureID::Invalid;
    }

    const ImageCreateInfo createInfo = {
        .width = info.width,
        .height = info.height,
        .arrayLayers = info.arrayLayers,
        .mipLevels = info.mipLevels,
        .type = info.arrayLayers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .format = format,
    };

    Image image = renderDevice->createImage(createInfo);
    renderDevice->uploadImage(image, info);
    freeImageInfo(info);

    // generate mipmaps for non ktx images
    if (!info.textureKTX && createInfo.mipLevels > 1)
        renderDevice->generateMipmaps(image);

    TextureID id = (TextureID)textures.size();
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

bool TextureManager::loadImageInfo(std::filesystem::path filepath, ImageLoadInfo &info, bool flip)
{
    if (!std::filesystem::exists(filepath))
        return false;

    if (String(filepath).ends_with(".ktx")) { // compressed ktx image
        // LOGI("Loading KTX texture: %s", filepath.c_str());
        ktxResult result;

        ktxTexture *textureKTX = nullptr;
        result = ktxTexture_CreateFromNamedFile(filepath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &textureKTX);
        if (result != KTX_SUCCESS)
            return false;

        info.channels = 4; // rgba
        info.width = textureKTX->baseWidth;
        info.height = textureKTX->baseHeight;
        info.mipLevels = textureKTX->numLevels;
        info.arrayLayers = textureKTX->numFaces;
        info.data = ktxTexture_GetData(textureKTX);
        info.size = ktxTexture_GetDataSize(textureKTX);
        info.textureKTX = textureKTX;
    } else { // other image types
        // LOGI("Loading regular texture: %s", filepath.c_str());

        int loadedChannels;
        info.data = stbi_load(filepath.c_str(), (int *)&info.width, (int *)&info.height, (int *)&loadedChannels, STBI_rgb_alpha);
        stbi_set_flip_vertically_on_load(flip);
        if (!info.data) {
            freeImageInfo(info);
            return false;
        }

        info.channels = STBI_rgb_alpha;
        info.arrayLayers = 1;
        info.size = info.width * info.height * STBI_rgb_alpha;
        info.mipLevels = renderDevice->calculateMipLevels(info.width, info.height);
    }

    return true;
}

bool TextureManager::loadImageInfo(unsigned char *data, uint32_t size, ImageLoadInfo &info, bool flip)
{
    int loadedChannels;
    info.data = stbi_load_from_memory(data, size, (int *)&info.width, (int *)&info.height, (int *)&loadedChannels, STBI_rgb_alpha);
    stbi_set_flip_vertically_on_load(flip);
    if (!info.data) {
        freeImageInfo(info);
        return false;
    }

    info.channels = STBI_rgb_alpha;
    info.arrayLayers = 1;
    info.size = info.width * info.height * STBI_rgb_alpha;
    info.mipLevels = renderDevice->calculateMipLevels(info.width, info.height);

    return true;
}

void TextureManager::freeImageInfo(ImageLoadInfo &info)
{
    if (info.textureKTX) {
        ktxTexture_Destroy(info.textureKTX);
    } else {
        stbi_image_free(info.data);
    }
}