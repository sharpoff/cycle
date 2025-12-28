#ifndef MATERIALS_GLSL
#define MATERIALS_GLSL

#include "types.glsl"

layout (set = 0, binding = 3) readonly buffer MaterialsBuffer
{
    Material materials[];
};

#endif