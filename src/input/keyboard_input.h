#pragma once

#include "SDL3/SDL_events.h"
#include "core/containers.h"

class KeyboardInput
{
public:
    void processEvent(SDL_Event *event);

    bool wasJustPressed(SDL_Keycode scancode);
    bool isPressed(SDL_Keycode scancode);
    bool isReleased(SDL_Keycode scancode);

private:
    struct KeyState
    {
        bool current = false;
        bool previous = false;
    };

    UnorderedMap<SDL_Keycode, KeyState> keys{};
};