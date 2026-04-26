#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

#include "types.glsl"

layout (set = 0, binding = 4) readonly buffer LightsBuffer
{
    Light lights[];
};

#endif