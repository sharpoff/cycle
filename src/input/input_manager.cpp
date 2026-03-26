#include "input/input_manager.h"

#include <assert.h>

void InputManager::Init()
{
}

void InputManager::Shutdown()
{
    delete gInput;
}

void InputManager::Update()
{
    mouseInput_.Update();
}

void InputManager::ProcessEvent(SDL_Event *event)
{
    assert(event);
    keyboardInput_.ProcessEvent(event);
    mouseInput_.ProcessEvent(event);
    gamepadInput_.ProcessEvent(event);
}