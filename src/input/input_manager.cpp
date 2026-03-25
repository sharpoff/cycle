#include "input/input_manager.h"

#include <assert.h>

void InputManager::init()
{
}

void InputManager::shutdown()
{
}

void InputManager::update()
{
    mouseInput_.update();
}

void InputManager::processEvent(SDL_Event *event)
{
    assert(event);
    keyboardInput_.processEvent(event);
    mouseInput_.processEvent(event);
    gamepadInput_.processEvent(event);
}