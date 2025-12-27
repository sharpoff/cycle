#pragma once

#include <stdint.h>

enum RenderingFlagBits : uint8_t
{
    RENDERING_NONE = 0,
    RENDERING_FILL = 1,
    RENDERING_WIREFRAME = 2,
    RENDERING_LIGHTING = 1 << 1,
    RENDERING_SHADOWS = 1 << 2,
    RENDERING_MATERIAL = 1 << 3,
    RENDERING_ALL = RENDERING_FILL | RENDERING_LIGHTING | RENDERING_SHADOWS | RENDERING_MATERIAL
};
using RenderingFlags = uint32_t;