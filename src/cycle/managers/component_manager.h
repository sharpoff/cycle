#pragma once

#include "cycle/containers.h"
#include "cycle/logger.h"
#include "cycle/types/id.h"

#include <assert.h>

template<typename T>
class ComponentManager
{
public:
    T &addComponent(const EntityID entity)
    {
        if (entity == EntityID::Invalid) {
            LOGE("%s", "Failed to add component to invalid EntityID");
            return emptyComponent;
        }

        if (T *component = getComponent(entity); component != nullptr) {
            return *component;
        }

        T &component = components.emplace_back();
        entityComponentMap[entity] = components.size() - 1;
        entities.push_back(entity);
        return component;
    }

    T *getComponent(const EntityID entity)
    {
        if (entity == EntityID::Invalid)
            return nullptr;

        auto it = entityComponentMap.find(entity);
        if (it != entityComponentMap.end()) {
            return &components[it->second];
        }

        return nullptr;
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

    T emptyComponent{};

    UnorderedMap<EntityID, size_t> entityComponentMap;
};