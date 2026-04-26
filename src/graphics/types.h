#pragma once
#include "math/math_types.h"

struct Vertex
{
    vec3  position;
    float uv_x;
    vec3  normal;
    float uv_y;
    vec4 tangent;
};

struct GPULight
{
    vec3 position;
    float intensity;
    vec3 direction;
    unsigned int type;
    vec3 color;
    float _pad0;
};

struct GPUMaterial
{
    uint32_t baseColorTexID = UINT32_MAX;
    uint32_t metallicRoughnessTexID = UINT32_MAX;
    uint32_t normalTexID = UINT32_MAX;
    uint32_t emissiveTexID = UINT32_MAX;
    float roughnessFactor = 0.0f;
    float metallicFactor = 0.0f;
};