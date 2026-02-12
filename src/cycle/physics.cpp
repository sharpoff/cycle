#include "cycle/physics.h"

#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/CollisionCollectorImpl.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/RegisterTypes.h"

#include "cycle/math_jolt.h"

#include "cycle/managers/entity_manager.h"
#include "cycle/managers/model_manager.h"

Physics *g_physics;

void Physics::init()
{
    static Physics instance;
    g_physics = &instance;

    instance.initInternal();
}

Physics *Physics::get()
{
    return g_physics;
}

void Physics::initInternal()
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

void Physics::shutdown()
{
    JPH::UnregisterTypes();

    delete tempAllocator;
    delete jobSystem;
}

void Physics::createBodies()
{
    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

    Vector<JPH::BodyID> newBodyIDs;

    for (const EntityID entityID : EntityManager::get()->rigidBodies.getEntities()) {
        RigidBodyComponent *rigidBodyComponent = EntityManager::get()->rigidBodies.getComponent(entityID);
        TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(entityID);
        if (!rigidBodyComponent || !transformComponent)
            continue;

        JPH::Shape::ShapeResult result;
        switch (rigidBodyComponent->type) {
            case RigidBodyType::Box: {
                JPH::BoxShapeSettings settings(JPH::Vec3Arg(math::toJolt(rigidBodyComponent->halfExtent)));
                settings.SetEmbedded();
                result = settings.Create();
                break;
            }
            case RigidBodyType::Sphere: {
                JPH::SphereShapeSettings settings(rigidBodyComponent->radius);
                settings.SetEmbedded();
                result = settings.Create();
                break;
            }
            case RigidBodyType::FromModel: {
                ModelComponent *modelComponent = EntityManager::get()->models.getComponent(entityID);
                if (!modelComponent)
                    break;

                Model *model = ModelManager::get()->getModelByID(modelComponent->modelID);
                if (!model)
                    break;

                JPH::Array<JPH::Vec3> points;
                for (Mesh &mesh : model->meshes) {
                    assert(mesh.indices.size() > 0 && mesh.indices.size() % 3 == 0);
                    for (size_t i = 0; i < mesh.indices.size(); i++) {
                        vec3 pos = transformComponent->transform * vec4(mesh.vertices[mesh.indices[i]].position, 0.0f);
                        points.push_back(JPH::Vec3(math::toJolt(pos)));
                    }
                }

                JPH::ConvexHullShapeSettings settings(points);
                settings.SetEmbedded();
                result = settings.Create();
                break;
            }
        }

        JPH::Ref<JPH::Shape> shape;
        if (result.IsValid()) {
            shape = result.Get();
        } else {
            LOGE("Failed to create body shape for entityID - %lu", entityID);
            continue;
        }

        bool isDynamic = rigidBodyComponent->isDynamic;

        vec3 position = math::getPosition(transformComponent->transform);
        quat rotation = math::getRotation(transformComponent->transform);

        JPH::BodyCreationSettings createSettings = JPH::BodyCreationSettings(
            shape.GetPtr(),
            math::toJolt(position),
            math::toJolt(rotation),
            isDynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
            isDynamic ? PhysicsLayers::MOVING : PhysicsLayers::NON_MOVING
        );

        JPH::Body *body = bodyInterface.CreateBody(createSettings);
        if (!body) {
            LOGE("Failed to create body for entityID - %lu", entityID);
            continue;
        }

        JPH::BodyID bodyID = body->GetID();
        bodyIDs.push_back(bodyID);
        newBodyIDs.push_back(bodyID);

        if (isDynamic)
            bodyInterface.SetLinearVelocity(bodyID, JPH::Vec3(0.0f, -5.0f, 0.0f));

        entityBodyMap[entityID] = bodyIDs.size() - 1;
        bodyEntityMap[bodyID] = entityID;
    }

    JPH::BodyInterface::AddState addState = bodyInterface.AddBodiesPrepare(newBodyIDs.data(), newBodyIDs.size());
    bodyInterface.AddBodiesFinalize(newBodyIDs.data(), newBodyIDs.size(), addState, JPH::EActivation::Activate);
}

const EntityID Physics::castRay(vec3 origin, vec3 direction)
{
    JPH::RRayCast ray;
    ray.mOrigin = math::toJolt(origin);
    ray.mDirection = JPH::Vec3(math::toJolt(direction));

    JPH::RayCastSettings settings;
    settings.mBackFaceModeTriangles = JPH::EBackFaceMode::CollideWithBackFaces;

    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    physicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector);
    collector.Sort();

    bool hasHit = collector.HadHit();
    if (hasHit)
    {
        for (int i = 0; i < collector.mHits.size(); i++)
        {
            const JPH::RayCastResult &hit = collector.mHits[i];
            return bodyEntityMap[hit.mBodyID];
        }
    }

    return EntityID::Invalid;
}

void Physics::update()
{
    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
    const float deltaTime = 1.0f / 60.0f;

    bool activeBodyExist = false;
    for (const EntityID entityID : EntityManager::get()->rigidBodies.getEntities()) {
        RigidBodyComponent *rigidBodyComponent = EntityManager::get()->rigidBodies.getComponent(entityID);
        TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(entityID);
        if (!rigidBodyComponent || !transformComponent)
            continue;

        auto it = entityBodyMap.find(entityID);
        if (it == entityBodyMap.end())
            continue;

        JPH::BodyID &bodyID = bodyIDs[it->second];

        // move body if dinamic
        if (rigidBodyComponent->isDynamic && bodyInterface.IsActive(bodyID)) {
            activeBodyExist = true;

            vec3 position = math::fromJolt(bodyInterface.GetPosition(bodyID));
            quat rotation = math::fromJolt(bodyInterface.GetRotation(bodyID));

            transformComponent->transform = glm::translate(position) * mat4(rotation) * glm::scale(math::getScale(transformComponent->transform));
        }
    }

    if (activeBodyExist > 0) {
        physicsSystem.Update(deltaTime, 1, tempAllocator, jobSystem);
    }
}

void Physics::setBodyPosition(const EntityID entityID, const vec3 &position)
{
    JPH::BodyID bodyID = getBodyID(entityID);
    if (bodyID.IsInvalid())
        return;

    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

    bodyInterface.SetPosition(bodyID, math::toJolt(position), JPH::EActivation::Activate);
    bodyInterface.SetLinearVelocity(bodyID, JPH::Vec3(0.0f, -5.0f, 0.0f));
}

JPH::BodyID Physics::getBodyID(const EntityID entityID)
{
    auto it = entityBodyMap.find(entityID);
    if (it != entityBodyMap.end())
        return bodyIDs[it->second];

    return JPH::BodyID(JPH::BodyID::cInvalidBodyID);
}