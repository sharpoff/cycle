#include "cycle/managers/entity_manager.h"

EntityManager *g_entityManager = nullptr;

void EntityManager::init()
{
    static EntityManager instance;
    g_entityManager = &instance;
}

EntityManager *EntityManager::get()
{
    return g_entityManager;
}

const EntityID EntityManager::createEntity()
{
    if (!freeList.empty()) {
        size_t idx = freeList.front();
        freeList.pop();

        EntityID id = EntityID(idx);
        entities[idx] = id;
        return id;
    }

    EntityID id = (EntityID)entities.size();
    entities.push_back(id);
    return id;
}

void EntityManager::destroyEntity(const EntityID id)
{
    if (id == EntityID::Invalid || (size_t)id >= entities.size())
        return;

    transforms.deleteEntity(id);
    lights.deleteEntity(id);
    models.deleteEntity(id);
    names.deleteEntity(id);
    rigidBodies.deleteEntity(id);

    size_t idx = (size_t)id;
    entities[idx] = EntityID::Invalid;
    freeList.push(idx);
}

const EntityID EntityManager::getEntityIDByName(String name)
{
    // XXX: this search could be improved
    for (const EntityID entityID : names.getEntities()) {
        NameComponent *nameComponent = names.getComponent(entityID);
        if (!nameComponent)
            continue;

        if (nameComponent->name == name)
            return entityID;
    }

    return EntityID::Invalid;
}