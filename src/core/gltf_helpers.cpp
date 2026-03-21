#include "core/gltf_helpers.h"

namespace gltf
{
    bool convertToModel(Model &model, Scene &scene)
    {
        if (scene.nodes.empty())
            return false;

        for (auto &node : scene.nodes) {
            model.meshIDs.push_back(node.meshID);
        }
        model.bounds = scene.bounds;

        return true;
    }
} // namespace gltf