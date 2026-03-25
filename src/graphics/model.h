#pragma once

#include "graphics/mesh.h"
#include "math/bounds.h"

struct Model
{
    Bounds bounds;
    Vector<Mesh> meshes;
};