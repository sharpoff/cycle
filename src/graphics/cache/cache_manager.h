#pragma once

#include "graphics/cache/material_cache.h"
#include "graphics/cache/mesh_cache.h"
#include "graphics/cache/texture_cache.h"

class CacheManager
{
public:
    CacheManager(RenderDevice &renderDevice);

    void releaseAllResources();

    TextureCache &getTextureCache() { return textureCache; }
    MaterialCache &getMaterialCache() { return materialCache; }
    MeshCache &getMeshCache() { return meshCache; }

private:
    TextureCache textureCache;
    MaterialCache materialCache;
    MeshCache meshCache;
};