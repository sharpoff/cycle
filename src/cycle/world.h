#pragma once

#include "cycle/containers.h"
#include "cycle/types/entity.h"
#include "cycle/types/id.h"

class World
{
public:
    const EntityID addEntity(Entity *entity, String name = "");
    void release();

    void update();

    Entity *getEntityByName(String name);
    Entity *getEntityByID(const EntityID id);

    const EntityID getEntityIDByName(String name);
    Vector<EntityID> getEntitiesByType(EntityType type);
    Vector<Entity*> &getEntities() { return entities; }

private:
    Vector<Entity*> entities;
    UnorderedMap<String, EntityID> nameEntityMap;
};