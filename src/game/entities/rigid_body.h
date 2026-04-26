#pragma once

// clang-format off
#include <Jolt/Jolt.h>
// clang-format on
#include "Jolt/Physics/Body/BodyID.h"

#include "game/entities/physics_entity.h"
#include "physics/physics_shape.h"

class RigidBody : public PhysicsEntity
{
public:
    virtual ~RigidBody();

    // NOTE: don't forget to set transform before creating body
    virtual void CreateBodyFromShape(const PhysicsShape &shape) override;
    virtual void CreateBodyFromModel() override;

    virtual PhysicsBodyType GetBodyType() override { return PhysicsBodyType::Rigid; }
    const JPH::BodyID &GetBodyID() override { return bodyID; }

private:
    JPH::BodyID bodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
    bool isShapeCreated = false;
};