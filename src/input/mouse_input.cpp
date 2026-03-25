#include "mouse_input.h"

void MouseInput::update()
{
    SDL_GetRelativeMouseState(&position.x, &position.y);
}

void MouseInput::processEvent(SDL_Event *event)
{
    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN && event->type != SDL_EVENT_MOUSE_BUTTON_UP)
        return;

    buttons[event->button.button].previous = buttons[event->button.button].current;
    buttons[event->button.button].current = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
}

bool MouseInput::wasJustPressed(SDL_MouseButtonFlags scancode)
{
    return buttons[scancode].current && !buttons[scancode].previous;
}

bool MouseInput::isPressed(SDL_MouseButtonFlags scancode)
{
    return buttons[scancode].current;
}

bool MouseInput::isReleased(SDL_MouseButtonFlags scancode)
{
    return !buttons[scancode].current;
}