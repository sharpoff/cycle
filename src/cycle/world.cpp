#include "cycle/world.h"
#include "cycle/types/render_data.h"

const EntityID World::addEntity(Entity *entity, String name)
{
    if (EntityID id = getEntityIDByName(name); id != EntityID::Invalid)
        return id;

    EntityID id = (EntityID)entities.size();
    entities.push_back(entity);
    if (!name.empty())
        nameEntityMap[name] = id;

    return id;
}

void World::release()
{
    for (Entity *entity : entities) {
        if (entity)
            delete entity;
    }
}

Entity *World::getEntityByName(String name)
{
    if (EntityID id = getEntityIDByName(name); id != EntityID::Invalid)
        return entities[(size_t)id];

    return nullptr;
}

Entity *World::getEntityByID(const EntityID id)
{
    if (id == EntityID::Invalid || (size_t)id >= entities.size()) {
        return nullptr;
    }

    return entities[(size_t)id];
}

const EntityID World::getEntityIDByName(String name)
{
    if (!name.empty()) {
        auto it = nameEntityMap.find(name);
        if (it != nameEntityMap.end()) {
            return it->second;
        }
    }

    return EntityID::Invalid;
}

const Vector<RenderData> World::getRenderData()
{
    Vector<RenderData> renderData;

    for (Entity *entity : entities) {
        if (entity) {
            RenderData &rd = renderData.emplace_back();
            rd.transform = entity->transform;
            rd.renderingFlags = entity->renderingFlags;
            rd.modelID = entity->modelID;
        }
    }

    return renderData;
}