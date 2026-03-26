#include "mouse_input.h"

void MouseInput::Update()
{
    SDL_GetRelativeMouseState(&position.x, &position.y);
}

void MouseInput::ProcessEvent(SDL_Event *event)
{
    isMoving = false;
    switch (event->type) {
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            buttons[event->button.button].previous = buttons[event->button.button].current;
            buttons[event->button.button].current = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            isMoving = true;
            break;
    }
}

bool MouseInput::WasJustPressed(SDL_MouseButtonFlags scancode)
{
    return buttons[scancode].current && !buttons[scancode].previous;
}

bool MouseInput::IsPressed(SDL_MouseButtonFlags scancode)
{
    return buttons[scancode].current;
}

bool MouseInput::IsReleased(SDL_MouseButtonFlags scancode)
{
    return !buttons[scancode].current;
}