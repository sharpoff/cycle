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
    void release();

    const TextureID createTexture(std::filesystem::path filepath, String name = "");

    const TextureID getTextureIDByName(String name);
    Vector<Image> &getTextures() { return textures; }

private:
    TextureManager() {}
    TextureManager(TextureManager &) = delete;
    void operator=(TextureManager const &) = delete;

    bool loadImageInfo(std::filesystem::path filename, ImageLoadInfo &info, bool flip = false);
    void freeImageInfo(ImageLoadInfo &info);
    void freeImageInfos(Vector<ImageLoadInfo> &infos);

    Vector<Image> textures;
    UnorderedMap<String, TextureID> filenameTextureMap;
    UnorderedMap<String, TextureID> nameTextureMap;

    RenderDevice *renderDevice = nullptr;
};