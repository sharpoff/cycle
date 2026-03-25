#pragma once

#include "core/containers.h"

template <typename Observer>
class Notifier
{
public:
    void AddObserver(Observer *observer)
    {
        observers.push_back(observer);
    }

    void RemoveObserver(Observer *observer)
    {
        observers.remove_if([&observer](const Observer *otherObserver) -> bool {
            return &observer == &otherObserver;
        });
    }

    template <typename... Args, typename... Fn>
    void Notify(void (Observer::*func)(Fn...), Args &&...args)
    {
        for (Observer *observer : observers) {
            (observer->*func)(std::forward<Args>(args)...);
        }
    }

private:
    Vector<Observer *> observers;
};