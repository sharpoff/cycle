#pragma once

#include "cycle/types/id.h"
#include "cycle/types/rendering_flags.h"
#include "cycle/types/transform.h"
#include <stdint.h>

enum EntityType
{
};

struct Entity
{
    Entity(Transform transform = Transform(), RenderingFlags renderingFlags = RENDERING_ALL, ModelID modelID = ModelID::Invalid)
        : transform(transform), renderingFlags(renderingFlags), modelID(modelID)
    {}

    Transform transform;
    RenderingFlags renderingFlags;
    ModelID modelID;
    EntityType type;
};