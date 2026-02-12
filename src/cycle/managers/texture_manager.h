#pragma once

#include "cycle/containers.h"
#include "cycle/graphics/vulkan_types.h"
#include "cycle/graphics/render_device.h"
#include "cycle/types/id.h"
#include <filesystem>

class TextureManager
{
public:
    static void init(RenderDevice *renderDevice);
    static TextureManager *get();
    void release();

    const TextureID createTexture(std::filesystem::path filepath, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, String name = "");
    const TextureID createTextureFromMem(unsigned char *data, uint32_t size, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, String name = "");

    const TextureID getTextureIDByName(String name);
    Vector<Image> &getTextures() { return textures; }

private:
    TextureManager() {}
    TextureManager(TextureManager &) = delete;
    void operator=(TextureManager const &) = delete;

    bool loadImageInfo(std::filesystem::path filename, ImageLoadInfo &info, bool flip = false);
    bool loadImageInfo(unsigned char *data, uint32_t size, ImageLoadInfo &info, bool flip = false);

    void freeImageInfo(ImageLoadInfo &info);

    Vector<Image> textures;
    UnorderedMap<String, TextureID> filenameTextureMap;
    UnorderedMap<String, TextureID> nameTextureMap;

    RenderDevice *renderDevice = nullptr;
};