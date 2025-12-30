#include "cycle/world.h"
#include "cycle/types/render_data.h"

const EntityID World::addEntity(Entity *entity, String name)
{
    if (!name.empty()) {
        auto it = nameEntityMap.find(name);
        if (it != nameEntityMap.end()) {
            return it->second;
        }
    }

    EntityID id = (EntityID)entities.size();
    entities.push_back(entity);
    if (!name.empty())
        nameEntityMap[name] = id;

    RenderData &rd = renderData.emplace_back();
    rd.transform = entity->transform;
    rd.renderingFlags = entity->renderingFlags;
    rd.modelID = entity->modelID;

    return id;
}

void World::release()
{
    for (Entity *entity : entities) {
        if (entity)
            delete entity;
    }
}