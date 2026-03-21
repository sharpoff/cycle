#pragma once

#include "core/id.h"
#include "objects/object.h"

class World
{
public:
    const ObjectID addObject(Object *object, String name = "");

    // TODO: add free list
    bool freeObject(const ObjectID id);
    bool freeObject(String name);

    Object *getObject(const ObjectID id);
    Object *getObject(String name);
    Vector<Object *> getObjects() { return objects; }

private:
    Vector<Object *> objects;
    UnorderedMap<String, ObjectID> nameObjectIDMap;
};