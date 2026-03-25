#include "physics/physics.h"

#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "game/static_object.h"
#include "math/math_jolt.h"
#include "math/math_types.h"

#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/RegisterTypes.h"
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>

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
}

void Physics::shutdown()
{
    JPH::UnregisterTypes();

    delete tempAllocator;
    delete jobSystem;
}

StaticObject *Physics::createStaticObject(const PhysicsShape &physicsShape, const vec3 &position, const quat &rotation, float scale)
{
    auto *obj = new StaticObject(createStaticBody(physicsShape, position, rotation));
    obj->setPosition(position);
    obj->setRotation(rotation);
    obj->setScale(scale);
    return obj;
}

JPH::BodyID Physics::createStaticBody(const PhysicsShape &physicsShape, const vec3 &position, const quat &rotation)
{
    JPH::Shape::ShapeResult result = createShape(physicsShape);

    JPH::Ref<JPH::Shape> shape;
    if (result.IsValid()) {
        shape = result.Get();
    } else {
        LOGE("Failed to create body shape", NULL);
        return JPH::BodyID(JPH::BodyID::cInvalidBodyID);
    }

    JPH::BodyCreationSettings createSettings = JPH::BodyCreationSettings(
        shape.GetPtr(),
        JPH::RVec3(math::toJolt(position)),
        math::toJolt(rotation),
        JPH::EMotionType::Static,
        PhysicsLayers::NON_MOVING);

    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
    bodyInterface.CreateAndAddBody(createSettings, JPH::EActivation::Activate);

    return JPH::BodyID(JPH::BodyID::cInvalidBodyID);
}

JPH::BodyID Physics::createDynamicBody(const PhysicsShape &physicsShape, const vec3 &position, const quat &rotation)
{
    JPH::Shape::ShapeResult result = createShape(physicsShape);

    JPH::Ref<JPH::Shape> shape;
    if (result.IsValid()) {
        shape = result.Get();
    } else {
        LOGE("Failed to create body shape", NULL);
        return JPH::BodyID(JPH::BodyID::cInvalidBodyID);
    }

    JPH::BodyCreationSettings createSettings = JPH::BodyCreationSettings(
        shape.GetPtr(),
        JPH::RVec3(math::toJolt(position)),
        math::toJolt(rotation),
        JPH::EMotionType::Dynamic,
        PhysicsLayers::MOVING);

    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();
    bodyInterface.CreateAndAddBody(createSettings, JPH::EActivation::Activate);

    return JPH::BodyID(JPH::BodyID::cInvalidBodyID);
}

void Physics::preUpdate()
{
    JPH::BodyInterface &bodyInterface = physicsSystem.GetBodyInterface();

    for (JPH::BodyID &bodyID : dynamicBodies) {
        if (bodyInterface.IsActive(bodyID)) {
            activeBodyExist = true;

            vec3 position = math::fromJolt(bodyInterface.GetPosition(bodyID));
            quat rotation = math::fromJolt(bodyInterface.GetRotation(bodyID));

            auto it = bodyObjectMap.find(bodyID);
            if (it != bodyObjectMap.end()) {
                Object *object = it->second;
                if (object) {
                    object->setPosition(position);
                    object->setRotation(rotation);
                }
            }
        }
    }
}

void Physics::update()
{
    const float deltaTime = 1.0f / 60.0f;

    if (activeBodyExist > 0) {
        physicsSystem.Update(deltaTime, 1, tempAllocator, jobSystem);
    }
}

void Physics::postUpdate()
{
}

JPH::Shape::ShapeResult Physics::createShape(const PhysicsShape &physicsShape)
{
    JPH::Shape::ShapeResult result;
    if (const BoxShape *box = std::get_if<BoxShape>(&physicsShape)) {
        JPH::BoxShapeSettings settings(JPH::Vec3Arg(math::toJolt(box->halfExtents)));
        settings.SetEmbedded();
        result = settings.Create();
    } else if (const SphereShape *sphere = std::get_if<SphereShape>(&physicsShape)) {
        JPH::SphereShapeSettings settings(sphere->radius);
        settings.SetEmbedded();
        result = settings.Create();
    } else if (const CapsuleShape *capsule = std::get_if<CapsuleShape>(&physicsShape)) {
        JPH::CapsuleShapeSettings settings(capsule->height / 2.0f, capsule->radius);
        settings.SetEmbedded();
        result = settings.Create();
    } else if (const ConvexHullShape *convexHull = std::get_if<ConvexHullShape>(&physicsShape)) {
        JPH::Array<JPH::Vec3> points;
        for (const vec3 point : convexHull->points) {
            points.push_back(math::toJolt(point));
        }

        JPH::ConvexHullShapeSettings settings(points);
        settings.SetEmbedded();
        result = settings.Create();
    }

    return result;
}