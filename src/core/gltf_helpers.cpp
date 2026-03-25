#include "core/gltf_helpers.h"

namespace gltf
{
    bool convertToModel(ModelPtr model, Scene &scene)
    {
        if (scene.nodes.empty())
            return false;

        for (auto &node : scene.nodes) {
            if (!node.mesh.primitives.empty())
                model->meshes.push_back(node.mesh);
        }
        model->bounds = scene.bounds;

        return true;
    }
} // namespace gltf