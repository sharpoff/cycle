#pragma once

#include "SDL3/SDL_events.h"
#include "core/containers.h"

class KeyboardInput
{
public:
    void ProcessEvent(SDL_Event *event);

    bool WasJustPressed(SDL_Keycode scancode);
    bool IsPressed(SDL_Keycode scancode);
    bool IsReleased(SDL_Keycode scancode);

private:
    struct KeyState
    {
        bool current = false;
        bool previous = false;
    };

    UnorderedMap<SDL_Keycode, KeyState> keys{};
};