#include "keyboard_input.h"

void KeyboardInput::ProcessEvent(SDL_Event *event)
{
    if (event->type != SDL_EVENT_KEY_DOWN && event->type != SDL_EVENT_KEY_UP)
        return;

    keys[event->key.key].previous = keys[event->key.key].current;
    keys[event->key.key].current = event->type != SDL_EVENT_KEY_UP;
}

bool KeyboardInput::WasJustPressed(SDL_Keycode scancode)
{
    return keys[scancode].current && !keys[scancode].previous;
}

bool KeyboardInput::IsPressed(SDL_Keycode keycode)
{
    return keys[keycode].current;
}

bool KeyboardInput::IsReleased(SDL_Keycode keycode)
{
    return !keys[keycode].current;
}