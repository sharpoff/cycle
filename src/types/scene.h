#pragma once

#include "core/containers.h"
#include "types/id.h"

struct SceneNode
{
    MeshID meshID = MeshID::Invalid;
    Vector<SceneNode> children;
};

struct Scene
{
    Vector<SceneNode> nodes;
};