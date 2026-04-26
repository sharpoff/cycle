#pragma once

#include <Jolt/Jolt.h>

#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "core/containers.h"

#include "physics/physics_layers.h"
#include "physics/physics_listeners.h"
#include "physics/physics_shape.h"

class PhysicsEntity;

class Physics
{
public:
    friend class Engine;

    void Initialize();
    void Shutdown();

    JPH::BodyID CreateStaticBody(const PhysicsShape &physicsShape, const vec3 &position = vec3(0.0f), const quat &rotation = glm::identity<quat>());
    JPH::BodyID CreateDynamicBody(const PhysicsShape &physicsShape, const vec3 &position = vec3(0.0f), const quat &rotation = glm::identity<quat>());

    void RegisterEntity(PhysicsEntity *entity);
    void UnregisterEntity(PhysicsEntity *entity);

    void PreUpdate();
    void Update();
    void PostUpdate();

private:
    Physics() {};
    Physics(const Physics &) = delete;
    Physics(Physics &&) = delete;
    Physics &operator=(const Physics &) = delete;
    Physics &operator=(Physics &&) = delete;

    JPH::Shape::ShapeResult CreateShape(const PhysicsShape &physicsShape);

    JPH::JobSystemThreadPool *jobSystem;
    JPH::TempAllocatorImpl *tempAllocator;

    // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
    // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
    const uint maxBodies = 1024;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
    const uint numBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase).
    const uint maxContactConstraints = 1024;

    const JPH::BodyID InvalidBodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);

    // Create mapping table from object layer to broadphase layer
    BPLayerInterfaceImpl broadPhaseLayerInterface;

    // Create class that filters object vs broadphase layers
    ObjectVsBroadPhaseLayerFilterImpl objectVsBroadPhaseLayerFilter;

    // Create class that filters object vs object layers
    ObjectLayerPairFilterImpl objectVsObjectFilter;

    JPH::PhysicsSystem physicsSystem;

    BodyActivationListener bodyActivationListener;

    ContactListener contactListener;

    bool activeBodyExist = false;

    Vector<PhysicsEntity*> registredEntities;
};

inline Physics *gPhysics = nullptr;