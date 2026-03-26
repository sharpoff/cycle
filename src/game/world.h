#pragma once

#include "game/entity.h"
#include "core/notifier.h"
#include "game/world_observer.h"

class World
{
public:
    friend class Application;

    void Init();
    void Shutdown();

    void Update(float deltaTime);

    void AddEntity(Entity *entity, String name);

    bool RemoveEntity(Entity *entity);
    bool RemoveEntityByName(String name);

    Entity *GetEntityByName(String name);
    Vector<Entity *> GetEntities() { return entities_; }

private:
    World() {}
    World(const World &) = delete;
    World(World &&) = delete;
    World &operator=(const World &) = delete;
    World &operator=(World &&) = delete;

    Vector<Entity *> entities_;
    UnorderedMap<String, uint32_t> nameObjectIDMap_;

    Notifier<WorldObserver> notifier;
};

inline World *gWorld = nullptr;