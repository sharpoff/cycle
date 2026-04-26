#include "keyboard_input.h"

void KeyboardInput::ProcessEvent(SDL_Event *event)
{
    if (event->type != SDL_EVENT_KEY_DOWN && event->type != SDL_EVENT_KEY_UP)
        return;

    m_keys[event->key.key].previous = m_keys[event->key.key].current;
    m_keys[event->key.key].current = event->type != SDL_EVENT_KEY_UP;
}

bool KeyboardInput::WasJustPressed(SDL_Keycode scancode)
{
    return m_keys[scancode].current && !m_keys[scancode].previous;
}

bool KeyboardInput::IsPressed(SDL_Keycode keycode)
{
    return m_keys[keycode].current;
}

bool KeyboardInput::IsReleased(SDL_Keycode keycode)
{
    return !m_keys[keycode].current;
}