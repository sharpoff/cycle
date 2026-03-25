#include "game/world.h"

#include <algorithm>

void World::init()
{
    // CacheManager &cacheManager = gRenderer->getCacheManager();
    // ModelCache &modelCache = cacheManager.getModelCache();
    // MeshCache &meshCache = cacheManager.getMeshCache();

    // {
    //     float scale = 0.01f;

    //     StaticObject *sponzaObject = physics->createStaticObject(convexHull, vec3(), glm::identity<quat>(), scale);
    //     sponzaObject->setModelID(modelCache.getModelIDByName("sponza"));
    //     addObject(sponzaObject, "sponza");
    // }

    // {
    //     const ModelID modelID = modelCache.getModelIDByName("monkey");
    //     ConvexHullShape convexHull = modelCache.getModelByID(modelID)->createConvexHullShape(meshCache);

    //     StaticObject *monkeyObject = physics->createStaticObject(convexHull, vec3(0, 5, 0));
    //     monkeyObject->setModelID(modelID);
    //     monkeyObject->setDrawFlags(Object::kVisible | Object::kCastShadows);
    //     addObject(monkeyObject, "monkey");
    // }

    // {
    //     // vec2 screenSize = gRenderer->getScreenSize();
    //     // float aspectRatio = float(screenSize.x) / screenSize.y;

    //     // Player *player = new Player(gInput, aspectRatio);
    //     // addObject(player);
    // }
}

void World::shutdown()
{
    for (Object *object : objects_) {
        removeObject(object);
    }
}

void World::update(float deltaTime)
{
    for (Object *object : objects_) {
        object->Update(deltaTime);
    }
}

void World::addObject(Object *object, String name)
{
    if (!object)
        return;

    auto it = nameObjectIDMap_.find(name);
    if (it != nameObjectIDMap_.end())
        return;

    objects_.push_back(object);
    nameObjectIDMap_[name] = objects_.size() - 1;
}

bool World::removeObject(Object *object)
{
    if (!object)
        return false;

    objects_.erase(std::remove_if(objects_.begin(), objects_.end(), [&object](const Object *otherObject) -> bool {
        return &object == &otherObject;
    }), objects_.end());

    delete object;
    object = nullptr;

    return true;
}

bool World::removeObjectByName(String name)
{
    Object *obj = getObjectByName(name);
    return removeObject(obj);
}

Object *World::getObjectByName(String name)
{
    auto it = nameObjectIDMap_.find(name);
    if (it != nameObjectIDMap_.end())
        return objects_[(uint32_t)it->second];

    return nullptr;
}