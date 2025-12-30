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

    const Vector<RenderData> &getRenderData() { return renderData; }

private:
    Vector<Entity*> entities;
    UnorderedMap<String, EntityID> nameEntityMap;

    Vector<RenderData> renderData;
};