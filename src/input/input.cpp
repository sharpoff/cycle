#include "input/input.h"

#include <assert.h>

void Input::Initialize()
{
}

void Input::Shutdown()
{
    if (gInput)
        delete gInput;
}

void Input::Update()
{
    m_mouseInput.Update();
}

void Input::ProcessEvent(SDL_Event *event)
{
    assert(event);
    m_keyboardInput.ProcessEvent(event);
    m_mouseInput.ProcessEvent(event);
    m_gamepadInput.ProcessEvent(event);
}

bool Input::WasJustPressed(const String &actionName)
{
    return m_actionStates[actionName].wasJustPressed;
}

bool Input::IsPressed(const String &actionName)
{
    return m_actionStates[actionName].isPressed;
}

bool Input::IsReleased(const String &actionName)
{
    return m_actionStates[actionName].isReleased;
}

float Input::GetAxisState(const String &actionName)
{
    return m_actionStates[actionName].axisState;
}

void Input::RegisterKeyboardAction(const String &actionName, SDL_Keycode keyCode)
{
    m_actionStates[actionName] = ActionState{}; // XXX: if aldready registered, this will erase previous state
}

void Input::RegisterMouseAction(const String &actionName, SDL_MouseButtonFlags mouseButton)
{
    m_actionStates[actionName] = ActionState{};
}

void Input::RegisterGamepadButtonAction(const String &actionName, int button)
{
    m_actionStates[actionName] = ActionState{};
}

void Input::RegisterGamepadAxisAction(const String &actionName, int axis)
{
    m_actionStates[actionName] = ActionState{};
}