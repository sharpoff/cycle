#pragma once

#include "SDL3/SDL_events.h"

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
    static void init();
    static GamepadInput *get();
    void processEvent(SDL_Event *event);

    bool isConnected() { return gamepad != nullptr; }
    const GamepadState &getGamepadState() { return state; }

private:
    SDL_Gamepad *gamepad;
    GamepadState state;
};