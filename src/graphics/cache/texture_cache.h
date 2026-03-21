#pragma once

#include "core/containers.h"
#include "graphics/vulkan_types.h"
#include "graphics/render_device.h"
#include "graphics/id.h"
#include <filesystem>

class TextureCache
{
public:
    TextureCache(RenderDevice &renderDevice);

    void init();
    void release();

    const TextureID loadFromFile(std::filesystem::path filepath, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, String name = "");
    const TextureID loadFromMem(unsigned char *data, uint32_t size, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, String name = "");

    const TextureID getTextureIDByName(String name);
    Vector<Image> &getTextures() { return textures; }

private:
    TextureCache(TextureCache &) = delete;
    void operator=(TextureCache const &) = delete;

    bool loadImageInfo(std::filesystem::path filename, ImageLoadInfo &info, bool flip = false);
    bool loadImageInfo(unsigned char *data, uint32_t size, ImageLoadInfo &info, bool flip = false);

    void freeImageInfo(ImageLoadInfo &info);

    Vector<Image> textures;
    UnorderedMap<String, TextureID> filenameTextureMap;
    UnorderedMap<String, TextureID> nameTextureMap;

    RenderDevice &renderDevice;
};