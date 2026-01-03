#include "cycle/world.h"

#include "cycle/globals.h"

const EntityID World::addEntity(Entity *entity, String name)
{
    if (!entity)
        return EntityID::Invalid;

    if (EntityID id = getEntityIDByName(name); id != EntityID::Invalid)
        return id;

    EntityID id = (EntityID)entities.size();
    entities.push_back(entity);
    if (!name.empty()) {
        entity->name = name;
        nameEntityMap[name] = id;
    }

    return id;
}

void World::release()
{
    for (Entity *entity : entities) {
        if (entity)
            delete entity;
    }
}

void World::update()
{
    auto time = g_engine->getTime();

    for (Entity *entity : entities) {
        if (entity && entity->type == ENTITY_TYPE_LIGHT) {
            vec3 pos = entity->transform.getPosition();
            pos.x = sin(glm::radians(time * 360.0f) * 0.2f) * 5.0f;
            pos.z = cos(glm::radians(time * 360.0f) * 0.2f) * 5.0f;
            entity->transform.setPosition(pos);
        }
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

Vector<EntityID> World::getEntitiesByType(EntityType type)
{
    Vector<EntityID> result;
    for (size_t i = 0; i < entities.size(); i++) {
        Entity *entity = entities[i];
        if (entity && entity->type == type) {
            result.push_back((EntityID)i);
        }
    }

    return result;
}