#pragma once

#include "math/math_types.h"

struct MeshPushConstants
{
    mat4 worldMatrix = mat4(1.0f);
    uint64_t vertexBufferAddress;
    unsigned int materialId;
};

struct DepthPushConstants
{
    mat4 worldMatrix = mat4(1.0f);
    uint64_t vertexBufferAddress;
    unsigned int cascadeIndex;
};