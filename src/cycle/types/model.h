#pragma once

#include "cycle/types/id.h"
#include "cycle/math.h"
#include "cycle/containers.h"
#include "cycle/types/rendering_flags.h"

struct Model
{
    Vector<MeshID> meshes;
    mat4 worldMatrix = mat4(1.0f);
    RenderingFlags renderingFlags = RENDERING_ALL;
};