#pragma once

#include "cycle/containers.h"
#include "cycle/logger.h"
#include "cycle/types/id.h"

#include <assert.h>

template<typename T>
class ComponentHolder
{
public:
    T &addComponent(const EntityID entityID)
    {
        if (entityID == EntityID::Invalid) {
            LOGE("%s", "Failed to add component to invalid EntityID");
            return emptyComponent;
        }

        if (T *component = getComponent(entityID); component != nullptr) {
            return *component;
        }

        if (!freeList.empty()) {
            size_t idx = freeList.front();
            freeList.pop();

            T &component = components[idx];
            entityComponentMap[entityID] = idx;
            entities[idx] = entityID;
            return component;
        } else {
            T &component = components.emplace_back();
            entityComponentMap[entityID] = components.size() - 1;
            entities.push_back(entityID);
            return component;
        }
    }

    void deleteEntity(const EntityID entityID)
    {
        if (entityID == EntityID::Invalid)
            return;

        auto it = entityComponentMap.find(entityID);
        if (it != entityComponentMap.end()) {
            components[it->second] = emptyComponent;
            entities[it->second] = EntityID::Invalid;
            entityComponentMap.erase(entityID);
            freeList.push(it->second);
        }
    }

    T *getComponent(const EntityID entityID)
    {
        if (entityID == EntityID::Invalid)
            return nullptr;

        auto it = entityComponentMap.find(entityID);
        if (it != entityComponentMap.end()) {
            return &components[it->second];
        }

        return nullptr;
    }

    bool has(const EntityID entityID)
    {
        return getComponent(entityID) != nullptr;
    }

    Vector<EntityID> &getEntities()
    {
        return entities;
    }

    Vector<T> &getComponents()
    {
        return components;
    }

private:
    Vector<EntityID> entities;
    Vector<T> components;

    Queue<size_t> freeList;

    T emptyComponent{};

    UnorderedMap<EntityID, size_t> entityComponentMap;
};