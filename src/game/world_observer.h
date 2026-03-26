#pragma once

class Entity;

class WorldObserver
{
public:
    virtual ~WorldObserver() = 0;
    virtual void OnAddObject(Entity *object) = 0;
    virtual void OnRemoveObject(Entity *object) = 0;
};