#pragma once

#include "core/containers.h"
#include <filesystem>
#include "core/gltf_scene.h"

#include "cgltf.h"

class CacheManager;

namespace gltf
{
    class Loader
    {
    public:
        static bool load(CacheManager &cacheManager, Scene &scene, std::filesystem::path filename);

    private:
        static void processNode(CacheManager &cacheManager, SceneNode &node, cgltf_data *data, cgltf_node *gltfNode, String baseDir);

        // claculates scene bounds from meshes vertices
        static bool calculateBounds(Scene &scene, CacheManager &cacheManager);
    };
} // namespace gltf