#pragma once

#include "game/entity.h"
#include "physics/physics_shape.h"

namespace JPH { class BodyID; }

enum class PhysicsBodyType
{
    Static,
    Rigid
};

class PhysicsEntity : public Entity
{
public:
    virtual ~PhysicsEntity() = default;

    virtual void CreateBodyFromShape(const PhysicsShape &shape) = 0;
    virtual void CreateBodyFromModel() = 0;

    virtual PhysicsBodyType GetBodyType() = 0;

    virtual const JPH::BodyID &GetBodyID() = 0;
};