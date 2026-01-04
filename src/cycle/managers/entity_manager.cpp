#include "cycle/managers/entity_manager.h"

#include "cycle/globals.h"

void EntityManager::init()
{
    static EntityManager entityManager;
    g_entityManager = &entityManager;
}

const EntityID EntityManager::createEntity()
{
    // TODO: implement free list and take entity from it?
    EntityID id = (EntityID)entities.size();
    entities.push_back(id);
    return id;
}