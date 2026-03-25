#pragma once

#include "SDL3/SDL_events.h"
#include "core/containers.h"
#include "math/math_types.h"

class MouseInput
{
public:
    void Update();
    void ProcessEvent(SDL_Event *event);

    bool WasJustPressed(SDL_MouseButtonFlags scancode);
    bool IsPressed(SDL_MouseButtonFlags scancode);
    bool IsReleased(SDL_MouseButtonFlags scancode);

    vec2 &GetRelativePosition() { return position; }

private:
    struct MouseState
    {
        bool current = false;
        bool previous = false;
    };

    UnorderedMap<SDL_MouseButtonFlags, MouseState> buttons{};
    vec2 position = vec2(0.0f);
};