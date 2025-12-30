#pragma once

#include "cycle/containers.h"
#include "cycle/types/entity.h"
#include "cycle/types/id.h"
#include "cycle/types/render_data.h"

class World
{
public:
    const EntityID addEntity(Entity *entity, String name = "");
    void release();

    Entity *getEntityByName(String name);
    Entity *getEntityByID(const EntityID id);

    const EntityID getEntityIDByName(String name);

    const Vector<RenderData> getRenderData();

private:
    Vector<Entity*> entities;
    UnorderedMap<String, EntityID> nameEntityMap;
};