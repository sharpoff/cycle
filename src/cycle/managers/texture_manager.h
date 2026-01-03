#pragma once

#include "cycle/containers.h"
#include "cycle/graphics/vulkan_types.h"
#include "cycle/graphics/render_device.h"
#include "cycle/types/id.h"

class TextureManager
{
public:
    static void init(RenderDevice *renderDevice);
    void release();

    const TextureID createTexture2D(String filename, String name = "");
    const TextureID createTextureCubeMap(String directory, String name = "");

    const TextureID getTextureIDByName(String name);
    Vector<Image> &getTextures() { return textures; }

private:
    bool loadImageInfoFromFile(String filename, ImageLoadInfo &info, bool flip = false);
    void freeImageInfo(ImageLoadInfo &info);
    void freeImageInfos(Vector<ImageLoadInfo> &infos);

    Vector<Image> textures;
    UnorderedMap<String, TextureID> filenameTextureMap;
    UnorderedMap<String, TextureID> nameTextureMap;

    RenderDevice *renderDevice = nullptr;
};