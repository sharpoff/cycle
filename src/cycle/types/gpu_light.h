#pragma once

#include "cycle/math.h"

struct GPULight
{
    vec3 position;
    float intensity;
    vec3 direction;
    unsigned int type;
    vec3 color;
    float _pad0;
};