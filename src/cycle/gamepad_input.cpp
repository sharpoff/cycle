#include "cycle/gamepad_input.h"
#include "cycle/logger.h"

#include <assert.h>

GamepadInput *g_gamepadInput;

void GamepadInput::init()
{
    static GamepadInput instance;
    g_gamepadInput = &instance;
}

void GamepadInput::processEvent(SDL_Event *event)
{
    assert(event);

    if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
        LOGI("%s", "Gamepad added.");
        if (!gamepad) {
            gamepad = SDL_OpenGamepad(event->gdevice.which);
        }
    } else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        LOGI("%s", "Gamepad removed.");
        if (gamepad) {
            SDL_CloseGamepad(gamepad);
            gamepad = nullptr;
        }
    } else if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        /* left thumb axis. */
        state.leftAxisX = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        state.leftAxisY = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);

        /* right thumb axis. */
        state.rightAxisX = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        state.rightAxisY = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
    } else if ((event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) || (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)) {
        state.buttonA = event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH && event->gbutton.down;
        state.buttonB = event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST && event->gbutton.down;
        state.buttonX = event->gbutton.button == SDL_GAMEPAD_BUTTON_WEST && event->gbutton.down;
        state.buttonY = event->gbutton.button == SDL_GAMEPAD_BUTTON_NORTH && event->gbutton.down;
    }
}