#pragma once

#include "cycle/math.h"

struct SceneInfo
{
    mat4 viewProjection;
};

struct MeshDrawInfo
{
    mat4 worldMatrix = mat4(1.0f);
    uint64_t vertexBufferAddress;
};