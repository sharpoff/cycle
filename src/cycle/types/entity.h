#pragma once

#include "cycle/containers.h"
#include "cycle/types/id.h"
#include "cycle/types/render_data.h"
#include "cycle/types/rendering_flags.h"
#include "cycle/types/transform.h"

enum EntityType
{
    ENTITY_TYPE_NONE = 0,
    ENTITY_TYPE_LIGHT
};

struct Entity
{
    Entity(Transform transform = Transform(), ModelID modelID = ModelID::Invalid, RenderingFlags renderingFlags = RENDERING_ALL)
        : transform(transform), renderingFlags(renderingFlags), modelID(modelID)
    {}

    RenderData getRenderData() {
        RenderData rd = {};
        rd.transform = transform;
        rd.renderingFlags = renderingFlags;
        rd.modelID = modelID;
        return rd;
    }

    String name = "";
    Transform transform;
    RenderingFlags renderingFlags;
    ModelID modelID;
    EntityType type = ENTITY_TYPE_NONE;
};