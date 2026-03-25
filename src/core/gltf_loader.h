#pragma once

#include "core/containers.h"
#include "core/filesystem.h"
#include "core/gltf_scene.h"

#include "cgltf.h"

class CacheManager;

namespace gltf
{
    class Loader
    {
    public:
        static bool Load(Scene &scene, FilePath filename);

    private:
        static void ProcessNode(SceneNode &node, cgltf_data *data, cgltf_node *gltfNode, String baseDir);

        // claculates scene bounds from meshes vertices
        static bool CalculateBounds(Scene &scene);
    };
} // namespace gltf