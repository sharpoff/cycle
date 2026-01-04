#pragma once

#include "cycle/math.h"

enum class RigidBodyType
{
    Box,
    Sphere,
};

struct RigidBodyComponent
{
    bool isDynamic = false;
    vec3 halfExtent = vec3(1.0f);
    RigidBodyType type = RigidBodyType::Box;
};