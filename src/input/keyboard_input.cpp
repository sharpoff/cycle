#include "keyboard_input.h"

void KeyboardInput::processEvent(SDL_Event *event)
{
    if (event->type != SDL_EVENT_KEY_DOWN && event->type != SDL_EVENT_KEY_UP)
        return;

    keys[event->key.key].previous = keys[event->key.key].current;
    keys[event->key.key].current = event->type != SDL_EVENT_KEY_UP;
}

bool KeyboardInput::wasJustPressed(SDL_Keycode scancode)
{
    return keys[scancode].current && !keys[scancode].previous;
}

bool KeyboardInput::isPressed(SDL_Keycode keycode)
{
    return keys[keycode].current;
}

bool KeyboardInput::isReleased(SDL_Keycode keycode)
{
    return !keys[keycode].current;
}