#include "mouse_input.h"

void MouseInput::Update()
{
    SDL_GetRelativeMouseState(&m_position.x, &m_position.y);
}

void MouseInput::ProcessEvent(SDL_Event *event)
{
    m_moving = false;
    switch (event->type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            m_buttons[event->button.button].previous = m_buttons[event->button.button].current;
            m_buttons[event->button.button].current = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            m_moving = true;
            break;
    }
}

void MouseInput::RegisterAction(const String &actionName, SDL_MouseButtonFlags scancode)
{
    m_mouseActions[actionName].push_back(scancode);
}

bool MouseInput::WasJustPressed(SDL_MouseButtonFlags scancode)
{
    return m_buttons[scancode].current && !m_buttons[scancode].previous;
}

bool MouseInput::IsPressed(SDL_MouseButtonFlags scancode)
{
    return m_buttons[scancode].current;
}

bool MouseInput::IsReleased(SDL_MouseButtonFlags scancode)
{
    return !m_buttons[scancode].current;
}