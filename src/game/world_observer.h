#pragma once

class Object;

class WorldObserver
{
public:
    virtual ~WorldObserver() = 0;
    virtual void OnAddObject(Object *object) = 0;
    virtual void OnRemoveObject(Object *object) = 0;
};