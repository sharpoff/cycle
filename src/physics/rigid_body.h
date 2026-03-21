#pragma once

#include "math/math_types.h"

struct RigidBody
{
    bool isDynamic = false;
};

struct RigidBodyBox : public RigidBody
{
    vec3 halfExtent;
};

struct RigidBodySphere : public RigidBody
{
    float radius;
};