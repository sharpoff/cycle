#pragma ocne

#include "math/math_types.h"

struct Cascade
{
    mat4 viewProjection = mat4(1.0f);
    float depth = 0.0f;

    float _pad0;
    float _pad1;
    float _pad2;
};