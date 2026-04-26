#include "input/gamepad_input.h"
#include "core/logger.h"

void GamepadInput::ProcessEvent(SDL_Event *event)
{
    if (event->type == SDL_EVENT_GAMEPAD_ADDED) {
        LOGI("Gamepad added");
        if (!m_gamepad) {
            m_gamepad = SDL_OpenGamepad(event->gdevice.which);
        }
    } else if (event->type == SDL_EVENT_GAMEPAD_REMOVED) {
        LOGI("Gamepad removed");
        if (m_gamepad) {
            SDL_CloseGamepad(m_gamepad);
            m_gamepad = nullptr;
        }
    } else if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        m_axes[SDL_GAMEPAD_AXIS_LEFTX] = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTX);
        m_axes[SDL_GAMEPAD_AXIS_LEFTY] = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFTY);
        m_axes[SDL_GAMEPAD_AXIS_RIGHTX] = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
        m_axes[SDL_GAMEPAD_AXIS_RIGHTY] = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHTY);
        m_axes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER] = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        m_axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER] = SDL_GetGamepadAxis(m_gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    } else if ((event->type == SDL_EVENT_GAMEPAD_BUTTON_UP) || (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN)) {
        bool down = event->gbutton.down;

        m_buttons[SDL_GAMEPAD_BUTTON_SOUTH].previous = m_buttons[SDL_GAMEPAD_BUTTON_SOUTH].current;
        m_buttons[SDL_GAMEPAD_BUTTON_SOUTH].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH;

        m_buttons[SDL_GAMEPAD_BUTTON_EAST].previous = m_buttons[SDL_GAMEPAD_BUTTON_EAST].current;
        m_buttons[SDL_GAMEPAD_BUTTON_EAST].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_EAST;

        m_buttons[SDL_GAMEPAD_BUTTON_WEST].previous = m_buttons[SDL_GAMEPAD_BUTTON_WEST].current;
        m_buttons[SDL_GAMEPAD_BUTTON_WEST].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_WEST;

        m_buttons[SDL_GAMEPAD_BUTTON_NORTH].previous = m_buttons[SDL_GAMEPAD_BUTTON_NORTH].current;
        m_buttons[SDL_GAMEPAD_BUTTON_NORTH].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_NORTH;

        m_buttons[SDL_GAMEPAD_BUTTON_BACK].previous = m_buttons[SDL_GAMEPAD_BUTTON_BACK].current;
        m_buttons[SDL_GAMEPAD_BUTTON_BACK].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_BACK;

        m_buttons[SDL_GAMEPAD_BUTTON_GUIDE].previous = m_buttons[SDL_GAMEPAD_BUTTON_GUIDE].current;
        m_buttons[SDL_GAMEPAD_BUTTON_GUIDE].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE;

        m_buttons[SDL_GAMEPAD_BUTTON_START].previous = m_buttons[SDL_GAMEPAD_BUTTON_START].current;
        m_buttons[SDL_GAMEPAD_BUTTON_START].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_START;

        m_buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK].previous = m_buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK].current;
        m_buttons[SDL_GAMEPAD_BUTTON_LEFT_STICK].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_STICK;

        m_buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK].previous = m_buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK].current;
        m_buttons[SDL_GAMEPAD_BUTTON_RIGHT_STICK].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_STICK;

        m_buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER].previous = m_buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER].current;
        m_buttons[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;

        m_buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER].previous = m_buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER].current;
        m_buttons[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;

        m_buttons[SDL_GAMEPAD_BUTTON_DPAD_UP].previous = m_buttons[SDL_GAMEPAD_BUTTON_DPAD_UP].current;
        m_buttons[SDL_GAMEPAD_BUTTON_DPAD_UP].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_UP;

        m_buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN].previous = m_buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN].current;
        m_buttons[SDL_GAMEPAD_BUTTON_DPAD_DOWN].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_DOWN;

        m_buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT].previous = m_buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT].current;
        m_buttons[SDL_GAMEPAD_BUTTON_DPAD_LEFT].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_LEFT;

        m_buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT].previous = m_buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT].current;
        m_buttons[SDL_GAMEPAD_BUTTON_DPAD_RIGHT].current = down && event->gbutton.button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT;
    }
}

bool GamepadInput::WasJustPressed(int button)
{
    return m_buttons[button].current && !m_buttons[button].previous;
}

bool GamepadInput::IsPressed(int button)
{
    return m_buttons[button].current;
}

bool GamepadInput::IsReleased(int button)
{
    return !m_buttons[button].current;
}

float GamepadInput::GetAxisState(int axis)
{
    return m_axes[axis];
}