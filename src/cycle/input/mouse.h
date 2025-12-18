#pragma once

#include <stdint.h>

enum class MouseButton : uint8_t
{
    RIGHT,
    LEFT,
    MIDDLE,

    COUNT,
    UNDEFINED,
};