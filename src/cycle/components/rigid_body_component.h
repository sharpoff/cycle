#pragma once

#include "cycle/math.h"

enum class RigidBodyType
{
    Box,
    Sphere,
    FromModel,
};

struct RigidBodyComponent
{
    bool isDynamic = false;
    RigidBodyType type = RigidBodyType::Box;
    union {
        vec3 halfExtent; // RigidBodyType::Box
        float radius; // RigidBodyType::Sphere
    };
};