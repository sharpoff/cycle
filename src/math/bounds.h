#pragma once

#include "math/math_types.h"

struct Bounds
{
    vec3 origin = vec3(0.0f);
    float sphereRadius = 0.0f;
    vec3 extents = vec3(0.0f);

    vec3 getMin() { return origin - extents; }
    vec3 getMax() { return origin + extents; }
    vec3 getHalfExtents() { return extents / 2.0f; }
};