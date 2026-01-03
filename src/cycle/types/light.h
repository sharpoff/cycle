#pragma once

#include "cycle/math.h"
#include "cycle/types/entity.h"
#include <stdint.h>

enum LightType : uint8_t
{
    LIGHT_TYPE_DIRECTIONAL = 0,
    LIGHT_TYPE_POINT = 1,
    LIGHT_TYPE_SPOT = 2,
};

struct Light : public Entity
{
    Light(LightType lightType, Transform transform = Transform(), ModelID modelID = ModelID::Invalid, RenderingFlags renderingFlags = RENDERING_ALL)
        : Entity(transform, modelID, renderingFlags), lightType(lightType)
    {
        this->type = ENTITY_TYPE_LIGHT;
    }

    LightType lightType = LIGHT_TYPE_DIRECTIONAL;
    vec3 color = vec3(1.0f);
};

struct LightGPU
{
    vec3 position = vec3(0.0f);
    LightType type = LIGHT_TYPE_DIRECTIONAL;
    vec3 color = vec3(1.0f);
};