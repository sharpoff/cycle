#pragma once

#include "SDL3/SDL_events.h"
#include "core/containers.h"

struct GamepadState
{
    float leftAxisX;
    float leftAxisY;

    float rightAxisX;
    float rightAxisY;

    bool buttonY = false;
    bool buttonA = false;
    bool buttonX = false;
    bool buttonB = false;

    float deadZone = 5100.0f;
};

class GamepadInput
{
public:
    void ProcessEvent(SDL_Event *event);

    // SDL_GamepadButton button
    bool WasJustPressed(int button);

    // SDL_GamepadButton button
    bool IsPressed(int button);

    // SDL_GamepadButton button
    bool IsReleased(int button);

    // SDL_GamepadAxis axis
    float GetAxisState(int axis);

    bool IsConnected() { return gamepad != nullptr; }

private:
    struct KeyState
    {
        bool current = false;
        bool previous = false;
    };

    SDL_Gamepad *gamepad = nullptr;
    UnorderedMap<int, float> axes; // SDL_GamepadAxis
    UnorderedMap<int, KeyState> buttons; // SDL_GamepadButton
};