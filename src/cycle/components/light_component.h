#pragma once

#include "cycle/math.h"

enum LightType : uint8_t
{
    LIGHT_TYPE_DIRECTIONAL = 0,
    LIGHT_TYPE_POINT = 1,
    LIGHT_TYPE_SPOT = 2,
};

struct LightComponent
{
    LightType lightType = LIGHT_TYPE_DIRECTIONAL;
    vec3 color = vec3(1.0f);
    vec3 direction = vec3(0.0f);
};