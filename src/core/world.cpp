#include "core/world.h"

#include "core/util.h"

const ObjectID World::addObject(Object *object, String name)
{
    if (!object)
        return ObjectID::Invalid;

    auto it = nameObjectIDMap.find(name);
    if (it != nameObjectIDMap.end())
        return it->second;

    objects.push_back(object);
    const ObjectID id = ObjectID(objects.size() - 1);
    nameObjectIDMap[name] = id;

    return id;
}

bool World::freeObject(const ObjectID id)
{
    Object *obj = getObject(id);
    if (!obj)
        return false;

    SAFE_FREE(obj);
    return true;
}

bool World::freeObject(String name)
{
    Object *obj = getObject(name);
    if (!obj)
        return false;

    SAFE_FREE(obj);
    return true;
}

Object *World::getObject(const ObjectID id)
{
    if (id != ObjectID::Invalid && (uint32_t)id < objects.size())
        return objects[(uint32_t)id];

    return nullptr;
}

Object *World::getObject(String name)
{
    auto it = nameObjectIDMap.find(name);
    if (it != nameObjectIDMap.end())
        return objects[(uint32_t)it->second];

    return nullptr;
}