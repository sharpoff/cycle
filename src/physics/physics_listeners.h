#pragma once

#include <Jolt/Jolt.h>
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/Physics/Collision/ContactListener.h"
#include "core/logger.h"

#define PHYSICS_DEBUG_OUTPUT

class ContactListener : public JPH::ContactListener
{
    virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override
    {
#ifdef PHYSICS_DEBUG_OUTPUT
        LOGI("[PHYSICS] Contact validate callback\n", NULL);
#endif

        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
    {
#ifdef PHYSICS_DEBUG_OUTPUT
        LOGI("[PHYSICS] Contact added\n", NULL);
#endif
    }

    virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
    {
#ifdef PHYSICS_DEBUG_OUTPUT
        LOGI("[PHYSICS] Contact persisted\n", NULL);
#endif
    }

    virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
    {
#ifdef PHYSICS_DEBUG_OUTPUT
        LOGI("[PHYSICS] Contact removed\n", NULL);
#endif
    }
};

class BodyActivationListener : public JPH::BodyActivationListener
{
public:
    virtual void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
    {
#ifdef PHYSICS_DEBUG_OUTPUT
        LOGI("[PHYSICS] Body activated\n", NULL);
#endif
    }

    virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
    {
#ifdef PHYSICS_DEBUG_OUTPUT
        LOGI("[PHYSICS] Body deactivated\n", NULL);
#endif
    }
};
