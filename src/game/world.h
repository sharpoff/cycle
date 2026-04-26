#pragma once

#include "game/entity.h"

class World
{
public:
    friend class Engine;

    void Initialize();
    void Shutdown();

    void Update(float deltaTime);

    void AddEntity(Entity *entity, const String &name);

    bool RemoveEntity(Entity *entity);
    bool RemoveEntityByName(const String &name);

    Entity *GetEntityByName(const String &name);
    Vector<Entity *> GetEntities() { return entities_; }

private:
    World() {}
    World(const World &) = delete;
    World(World &&) = delete;
    World &operator=(const World &) = delete;
    World &operator=(World &&) = delete;

    Vector<Entity *> entities_;
    UnorderedMap<String, uint32_t> nameObjectIDMap_;
};

inline World *gWorld = nullptr;