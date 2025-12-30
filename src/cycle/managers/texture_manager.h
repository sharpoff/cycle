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

    const TextureID createTexture(String filename, String name = "");

    Vector<Image> &getTextures() { return textures; }

private:
    Vector<Image> textures;
    UnorderedMap<String, TextureID> filenameTextureMap;

    RenderDevice *renderDevice = nullptr;
};