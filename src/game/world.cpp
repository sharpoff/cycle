#include "game/world.h"

#include <algorithm>

void World::Init()
{
}

void World::Shutdown()
{
    for (Entity *entity : entities_) {
        RemoveEntity(entity);
    }
    delete gWorld;
}

void World::Update(float deltaTime)
{
    for (Entity *entity : entities_) {
        entity->Update(deltaTime);
    }
}

void World::AddEntity(Entity *entity, String name)
{
    if (!entity || name.empty())
        return;

    auto it = nameObjectIDMap_.find(name);
    if (it != nameObjectIDMap_.end())
        return;

    entities_.push_back(entity);
    entity->SetName(name);
    nameObjectIDMap_[name] = entities_.size() - 1;
}

bool World::RemoveEntity(Entity *entity)
{
    if (!entity)
        return false;

    entities_.erase(std::remove_if(entities_.begin(), entities_.end(), [&entity](const Entity *otherObject) -> bool {
        return &entity == &otherObject;
    }), entities_.end());

    delete entity;
    entity = nullptr;

    return true;
}

bool World::RemoveEntityByName(String name)
{
    Entity *entity = GetEntityByName(name);
    return RemoveEntity(entity);
}

Entity *World::GetEntityByName(String name)
{
    auto it = nameObjectIDMap_.find(name);
    if (it != nameObjectIDMap_.end())
        return entities_[(uint32_t)it->second];

    return nullptr;
}