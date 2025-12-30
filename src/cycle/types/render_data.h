#pragma once

#include "cycle/types/id.h"
#include "cycle/types/rendering_flags.h"
#include "cycle/types/transform.h"

struct RenderData
{
    Transform transform = Transform();
    RenderingFlags renderingFlags = RENDERING_ALL;
    ModelID modelID = ModelID::Invalid;
};