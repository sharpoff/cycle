#pragma once

#include <Jolt/Jolt.h>
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/Physics/Collision/ContactListener.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "cycle/containers.h"
#include "cycle/math.h"
#include "cycle/types/id.h"

constexpr const bool enableDebugOutput = false;

//---------------------------
// Layers
//---------------------------

// Layer that objects can be in, determines which other objects it can collide with
namespace PhysicsLayers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
} // namespace Layers

// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override
	{
        switch (inLayer1) {
            case PhysicsLayers::NON_MOVING:
                return inLayer2 == PhysicsLayers::MOVING;
            case PhysicsLayers::MOVING:
                return true;
            default:
                printf("[PHYSICS] Invalid object layer\n");
                return false;
        }
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase.
namespace PhysicsBroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create mapping table from object to broad phase layer
        objectToBroadPhase[PhysicsLayers::NON_MOVING] = PhysicsBroadPhaseLayers::NON_MOVING;
        objectToBroadPhase[PhysicsLayers::MOVING] = PhysicsBroadPhaseLayers::MOVING;
    }

	virtual uint GetNumBroadPhaseLayers() const override
    {
        return PhysicsBroadPhaseLayers::NUM_LAYERS;
    }

	/// Convert an object layer to the corresponding broadphase layer
	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        return objectToBroadPhase[inLayer];
    }

	virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {
            case (static_cast<JPH::BroadPhaseLayer::Type>(PhysicsBroadPhaseLayers::NON_MOVING)):
                return "NON_MOVING";
            case (static_cast<JPH::BroadPhaseLayer::Type>(PhysicsBroadPhaseLayers::MOVING)):
                return "MOVING";
            default:
                printf("[PHYSICS] Invalid broad phase layer name\n");
                return "INVALID";
        }
    }

private:
    JPH::BroadPhaseLayer objectToBroadPhase[PhysicsLayers::NUM_LAYERS];
};

// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1) {
            case PhysicsLayers::NON_MOVING:
                return inLayer2 == PhysicsBroadPhaseLayers::MOVING;
            case PhysicsLayers::MOVING:
                return true;
            default:
                printf("[PHYSICS] Invalid object or broad phase layer\n");
                return false;
        }
    }
};

//---------------------------
// Listeners
//---------------------------

class ContactListener : public JPH::ContactListener
{
    virtual JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override
    {
        if (enableDebugOutput) {
            printf("[PHYSICS] Contact validate callback\n");
        }

        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

	virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings)  override
    {
        if (enableDebugOutput) {
            printf("[PHYSICS] Contact added\n");
        }
    }

	virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
    {
        if (enableDebugOutput) {
            printf("[PHYSICS] Contact persisted\n");
        }
    }

	virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
    {
        if (enableDebugOutput) {
            printf("[PHYSICS] Contact removed\n");
        }
    }
};

class BodyActivationListener : public JPH::BodyActivationListener
{
public:
	virtual void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
    {
        if (enableDebugOutput) {
            printf("[PHYSICS] Body activated\n");
        }
    }

	virtual void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
    {
        if (enableDebugOutput) {
            printf("[PHYSICS] Body deactivated\n");
        }
    }
};

//---------------------------
// Physics
//---------------------------

class Physics
{
public:
    static void init();
    void shutdown();

    // should be called *after* creating entities, to affect physics entity components
    void createBodies();
    
    // should be called *before* update
    const EntityID castRay(vec3 origin, vec3 direction);

    void update();

    void setBodyPosition(const EntityID entityID, const vec3 &position);

private:
    Physics() {}
    Physics(Physics &) = delete;
    void operator=(Physics const &) = delete;

    void initInternal();

    JPH::BodyID getBodyID(const EntityID entityID);

    JPH::JobSystemThreadPool *jobSystem;
    JPH::TempAllocatorImpl   *tempAllocator;

    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const uint maxBodies = 1024;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const uint numBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase).
    const uint maxContactConstraints = 1024;

    // Create mapping table from object layer to broadphase layer
    BPLayerInterfaceImpl broadPhaseLayerInterface;

    // Create class that filters object vs broadphase layers
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;

    // Create class that filters object vs object layers
    ObjectLayerPairFilterImpl objectVsObjectFilter;

    JPH::PhysicsSystem physicsSystem;

    BodyActivationListener bodyActivationListener;

    ContactListener contactListener;

    Vector<JPH::BodyID> bodyIDs;

    UnorderedMap<EntityID, size_t> entityBodyMap;
    UnorderedMap<JPH::BodyID, EntityID> bodyEntityMap;
};