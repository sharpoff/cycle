#pragma once

#include "core/containers.h"
#include "math/math_types.h"
#include <variant>

struct BoxShape
{
    vec3 halfExtents;
};

struct SphereShape
{
    float radius;
};

struct CapsuleShape
{
    float radius;
    float height;
};

struct ConvexHullShape
{
    Vector<vec3> points;
};

using PhysicsShape = std::variant<
    BoxShape,
    SphereShape,
    CapsuleShape,
    ConvexHullShape>;