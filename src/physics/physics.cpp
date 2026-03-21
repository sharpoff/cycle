#include "physics/physics.h"

// #include "Jolt/Physics/Collision/CastResult.h"
// #include "Jolt/Physics/Collision/CollisionCollectorImpl.h"
// #include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/RegisterTypes.h"

// #include "math/math_jolt.h"

void PhysicsSystem::init()
{
    // Register allocation hook.
    JPH::RegisterDefaultAllocator();

    JPH::Factory::sInstance = new JPH::Factory();

    // Register all physics types with the factory and install their collision handlers with the CollisionDispatch class.
    JPH::RegisterTypes();

    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

    // We need a job system that will execute physics jobs on multiple threads.
    jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

    physicsSystem.Init(maxBodies, numBodyMutexes, maxBodies, maxContactConstraints, broadPhaseLayerInterface, objectVsBroadPhaseLayerFilter, objectVsObjectFilter);

    physicsSystem.SetBodyActivationListener(&bodyActivationListener);
    physicsSystem.SetContactListener(&contactListener);
}

void PhysicsSystem::shutdown()
{
    JPH::UnregisterTypes();

    delete tempAllocator;
    delete jobSystem;
}

void PhysicsSystem::update()
{
    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
    const float deltaTime = 1.0f / 60.0f;

    bool activeBodyExist = false;
    // for (const EntityID entityID : EntityManager::get()->rigidBodies.getEntities()) {
    //     RigidBodyComponent *rigidBodyComponent = EntityManager::get()->rigidBodies.getComponent(entityID);
    //     TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(entityID);
    //     if (!rigidBodyComponent || !transformComponent)
    //         continue;

    //     auto it = entityBodyMap.find(entityID);
    //     if (it == entityBodyMap.end())
    //         continue;

    //     JPH::BodyID &bodyID = bodyIDs[it->second];

    //     // move body if dinamic
    //     if (rigidBodyComponent->isDynamic && bodyInterface.IsActive(bodyID)) {
    //         activeBodyExist = true;

    //         vec3 position = math::fromJolt(bodyInterface.GetPosition(bodyID));
    //         quat rotation = math::fromJolt(bodyInterface.GetRotation(bodyID));

    //         transformComponent->transform = glm::translate(position) * mat4(rotation) * glm::scale(math::getScale(transformComponent->transform));
    //     }
    // }

    if (activeBodyExist > 0) {
        physicsSystem.Update(deltaTime, 1, tempAllocator, jobSystem);
    }
}