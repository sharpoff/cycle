#pragma once

#include "core/object.h"
#include "core/notifier.h"
#include "game/world_observer.h"

class World
{
public:
    friend class Application;

    void Init();
    void Shutdown();

    void Update(float deltaTime);

    void AddObject(Object *object, String name);

    bool RemoveObject(Object *object);
    bool RemoveObjectByName(String name);

    Object *GetObjectByName(String name);
    Vector<Object *> GetObjects() { return objects_; }

private:
    World() {}
    World(const World &) = delete;
    World(World &&) = delete;
    World &operator=(const World &) = delete;
    World &operator=(World &&) = delete;

    Vector<Object *> objects_;
    UnorderedMap<String, uint32_t> nameObjectIDMap_;

    Notifier<WorldObserver> notifier;
};

static World *gWorld = nullptr;