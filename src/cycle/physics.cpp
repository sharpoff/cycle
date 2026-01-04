#include "cycle/physics.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/RegisterTypes.h"

#include "cycle/globals.h"
#include "cycle/math_jolt.h"

void Physics::init()
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

    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

    for (const EntityID entityID : g_entityManager->rigidBodyComponents.getEntities()) {
        RigidBodyComponent *rigidBodyComponent = g_entityManager->rigidBodyComponents.getComponent(entityID);
        TransformComponent *transformComponent = g_entityManager->transformComponents.getComponent(entityID);
        if (!rigidBodyComponent || !transformComponent)
            continue;

        JPH::Ref<JPH::Shape> shape;

        bool success = false;
        switch (rigidBodyComponent->type) {
            case RigidBodyType::Box: {
                JPH::BoxShapeSettings settings(math::toJolt(rigidBodyComponent->halfExtent));
                settings.SetEmbedded();

                JPH::Shape::ShapeResult result = settings.Create();
                if (result.IsValid()) {
                    shape = result.Get();
                } else {
                    LOGE("%s", "Failed to create body shape!");
                    return;
                }

                success = true;
                break;
            }
            case RigidBodyType::Sphere: {
                LOGE("%s", "Failed RigidBodyType::Sphere - not implemented!");
                break;
            }
        }

        if (!success)
            continue;

        bool isDynamic = rigidBodyComponent->isDynamic;

        JPH::BodyID &bodyID = bodyIDs.emplace_back();
        vec3 position = math::getPosition(transformComponent->transform);
        quat rotation = math::getRotation(transformComponent->transform);

        JPH::BodyCreationSettings createSettings = JPH::BodyCreationSettings(
            shape.GetPtr(),
            math::toJolt(position),
            math::toJolt(rotation),
            isDynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
            isDynamic ? PhysicsLayers::MOVING : PhysicsLayers::NON_MOVING
        );

        bodyID = bodyInterface.CreateAndAddBody(createSettings, isDynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);

        if (isDynamic)
            bodyInterface.SetLinearVelocity(bodyID, JPH::Vec3(0.0f, -5.0f, 0.0f));

        entityBodyMap[entityID] = bodyIDs.size() - 1;
    }
}

void Physics::shutdown()
{
    JPH::UnregisterTypes();

    delete tempAllocator;
    delete jobSystem;
}

void Physics::update()
{
    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
    const float deltaTime = 1.0f / 60.0f;

    bool activeBodyExist = false;
    for (const EntityID entityID : g_entityManager->rigidBodyComponents.getEntities()) {
        RigidBodyComponent *rigidBodyComponent = g_entityManager->rigidBodyComponents.getComponent(entityID);
        TransformComponent *transformComponent = g_entityManager->transformComponents.getComponent(entityID);
        if (!rigidBodyComponent || !transformComponent)
            continue;

        auto it = entityBodyMap.find(entityID);
        if (it == entityBodyMap.end())
            continue;

        JPH::BodyID &bodyID = bodyIDs[it->second];

        // move body if dinamic
        if (rigidBodyComponent->isDynamic && bodyInterface.IsActive(bodyID)) {
            activeBodyExist = true;

            vec3 position = math::toMath(bodyInterface.GetPosition(bodyID));
            quat rotation = math::toMath(bodyInterface.GetRotation(bodyID));

            transformComponent->transform = glm::translate(position) * mat4(rotation) * glm::scale(math::getScale(transformComponent->transform));
        }
    }

    if (activeBodyExist > 0) {
        physicsSystem.Update(deltaTime, 1, tempAllocator, jobSystem);
    }
}