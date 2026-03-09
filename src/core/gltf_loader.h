#pragma once

#include <cgltf.h>
#include "types/scene.h"
#include <filesystem>

class CacheManager;

class GLTFLoader
{
public:
    static bool load(CacheManager &cacheManager, Scene &scene, std::filesystem::path filename);

private:
    static void processNode(CacheManager &cacheManager, SceneNode &node, cgltf_data *data, cgltf_node *gltfNode, String baseDir);
};