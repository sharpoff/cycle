#pragma once

#include "core/containers.h"
#include "graphics/mesh.h"
#include "math/bounds.h"

namespace gltf
{
    struct SceneNode
    {
        Mesh mesh;
        Vector<SceneNode> children;
    };

    struct Scene
    {
        Bounds bounds;
        Vector<SceneNode> nodes;
    };
} // namespace gltf