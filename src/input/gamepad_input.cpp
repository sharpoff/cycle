#include "gamepad_input.h"
#include "core/logger.h"

void GamepadInput::processEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
        LOGI("Gamepad added.", NULL);
        if (!gamepad) {
            gamepad = SDL_OpenGamepad(event->gdevice.which);
        }
    } else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        LOGI("Gamepad removed.", NULL);
        if (gamepad) {
            SDL_CloseGamepad(gamepad);
            gamepad = nullptr;
        }
    } else if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        axes[SDL_GAMEPAD_AXIS_LEFTX] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        axes[SDL_GAMEPAD_AXIS_LEFTY] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        axes[SDL_GAMEPAD_AXIS_RIGHTX] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        axes[SDL_GAMEPAD_AXIS_RIGHTY] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
        axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    } else if ((event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) || (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)) {
        bool down = event->gbutton.down;

        buttons[SDL_GAMEPAD_BUTTON_SOUTH].previous = buttons[SDL_GAMEPAD_BUTTON_SOUTH].current;
        buttons[SDL_GAMEPAD_BUTTON_SOUTH].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH;

        buttons[SDL_GAMEPAD_BUTTON_EAST].previous = buttons[SDL_GAMEPAD_BUTTON_EAST].current;
        buttons[SDL_GAMEPAD_BUTTON_EAST].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST;

        buttons[SDL_GAMEPAD_BUTTON_WEST].previous = buttons[SDL_GAMEPAD_BUTTON_WEST].current;
        buttons[SDL_GAMEPAD_BUTTON_WEST].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_WEST;

        buttons[SDL_GAMEPAD_BUTTON_NORTH].previous = buttons[SDL_GAMEPAD_BUTTON_NORTH].current;
        buttons[SDL_GAMEPAD_BUTTON_NORTH].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_NORTH;

        buttons[SDL_GAMEPAD_BUTTON_BACK].previous = buttons[SDL_GAMEPAD_BUTTON_BACK].current;
        buttons[SDL_GAMEPAD_BUTTON_BACK].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK;

        buttons[SDL_GAMEPAD_BUTTON_GUIDE].previous = buttons[SDL_GAMEPAD_BUTTON_GUIDE].current;
        buttons[SDL_GAMEPAD_BUTTON_GUIDE].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE;

        buttons[SDL_GAMEPAD_BUTTON_START].previous = buttons[SDL_GAMEPAD_BUTTON_START].current;
        buttons[SDL_GAMEPAD_BUTTON_START].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_START;

        buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK].previous = buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK].current;
        buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_STICK;

        buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK].previous = buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK].current;
        buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_STICK;

        buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER].previous = buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER].current;
        buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;

        buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER].previous = buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER].current;
        buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;

        buttons[SDL_GAMEPAD_BUTTON_DPAD_UP].previous = buttons[SDL_GAMEPAD_BUTTON_DPAD_UP].current;
        buttons[SDL_GAMEPAD_BUTTON_DPAD_UP].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP;

        buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN].previous = buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN].current;
        buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN;

        buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT].previous = buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT].current;
        buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT;

        buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT].previous = buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT].current;
        buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    }
}

bool GamepadInput::wasJustPressed(int button)
{
    return buttons[button].current && !buttons[button].previous;
}

bool GamepadInput::isPressed(int button)
{
    return buttons[button].current;
}

bool GamepadInput::isReleased(int button)
{
    return !buttons[button].current;
}

float GamepadInput::getAxisState(int axis)
{
    return axes[axis];
}