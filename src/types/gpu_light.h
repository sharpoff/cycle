#pragma once

#include "math/math_types.h"

struct GPULight
{
    vec3 position;
    float intensity;
    vec3 direction;
    unsigned int type;
    vec3 color;
    float _pad0;
};