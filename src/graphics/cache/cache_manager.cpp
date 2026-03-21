#include "graphics/cache/cache_manager.h"

#include "graphics/renderer.h"

CacheManager::CacheManager(Renderer &renderer)
    : textureCache(renderer.getRenderDevice()), meshCache(renderer.getRenderDevice()), modelCache(renderer)
{
}

void CacheManager::releaseAllResources()
{
    textureCache.release();
    meshCache.release();
}