#include "graphics/cache/cache_manager.h"

#include "graphics/render_device.h"

CacheManager::CacheManager(RenderDevice &renderDevice)
    : textureCache(renderDevice), materialCache(), meshCache(renderDevice)
{
}

void CacheManager::releaseAllResources()
{
    textureCache.release();
    meshCache.release();
}