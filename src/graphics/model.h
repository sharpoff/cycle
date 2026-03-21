#pragma once

#include "core/containers.h"
#include "graphics/id.h"
#include "math/bounds.h"

struct Model
{
    Bounds bounds;
    Vector<MeshID> meshIDs;
};