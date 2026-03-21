#pragma once

#include "core/containers.h"
#include "graphics/id.h"
#include "math/bounds.h"

namespace gltf
{
    struct SceneNode
    {
        MeshID meshID = MeshID::Invalid;
        Vector<SceneNode> children;
    };

    struct Scene
    {
        Bounds bounds;
        Vector<SceneNode> nodes;
    };
} // namespace gltf