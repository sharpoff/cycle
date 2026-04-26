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

    bool WasJustPressed(int button);
    bool IsPressed(int button);
    bool IsReleased(int button);

    float GetAxisState(int axis);

    bool IsConnected() { return m_gamepad != nullptr; }

private:
    struct KeyState
    {
        bool current = false;
        bool previous = false;
    };

    SDL_Gamepad *m_gamepad = nullptr;
    UnorderedMap<int, float> m_axes; // SDL_GamepadAxis
    UnorderedMap<int, KeyState> m_buttons; // SDL_GamepadButton

    UnorderedMap<String, Vector<int>> m_gamepadButtonActions;
    UnorderedMap<String, Vector<int>> m_gamepadAxisActions;
};