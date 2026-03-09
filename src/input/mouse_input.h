#pragma once

#include "SDL3/SDL_events.h"
#include "core/containers.h"

class MouseInput
{
public:
    void processEvent(SDL_Event *event);

    bool wasJustPressed(SDL_MouseButtonFlags scancode);
    bool isPressed(SDL_MouseButtonFlags scancode);
    bool isReleased(SDL_MouseButtonFlags scancode);

private:
    struct MouseState
    {
        bool current = false;
        bool previous = false;
    };

    UnorderedMap<SDL_MouseButtonFlags, MouseState> buttons{};
};