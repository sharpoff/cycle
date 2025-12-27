#pragma once

#include "cycle/id_types.h"
#include "cycle/math.h"
#include "cycle/types.h"
#include "cycle/types/rendering_flags.h"

struct Model
{
    Vector<MeshID> meshes;
    mat4 worldMatrix = mat4(1.0f);
    RenderingFlags renderingFlags = RENDERING_ALL;
};