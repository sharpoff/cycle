#pragma once

#include "cycle/math.h"
#include "cycle/types/entity.h"
#include <stdint.h>

enum LightType : uint8_t
{
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT,
    LIGHT_TYPE_SPOT,
};

struct Light : public Entity
{
    LightType type = LIGHT_TYPE_DIRECTIONAL;
    vec3 color = vec3(1.0f);
};