#pragma once

#include "input/action_state.h"
#include "input/gamepad_input.h"
#include "input/keyboard_input.h"
#include "input/mouse_input.h"

class Input
{
public:
    friend class Engine;

    void Initialize();
    void Shutdown();

    void Update();
    void ProcessEvent(SDL_Event *event);

    bool WasJustPressed(const String &actionName);
    bool IsPressed(const String &actionName);
    bool IsReleased(const String &actionName);

    float GetAxisState(const String &actionName);

    // TODO: implement it properly
    void RegisterKeyboardAction(const String &actionName, SDL_Keycode keyCode);
    void RegisterMouseAction(const String &actionName, SDL_MouseButtonFlags mouseButton);
    void RegisterGamepadButtonAction(const String &actionName, int button);
    void RegisterGamepadAxisAction(const String &actionName, int axis);

    KeyboardInput &GetKeyboardInput() { return m_keyboardInput; }
    MouseInput &GetMouseInput() { return m_mouseInput; }
    GamepadInput &GetGamepadInput() { return m_gamepadInput; }

private:
    Input() {}
    Input(const Input &) = delete;
    Input(Input &&) = delete;
    Input &operator=(const Input &) = delete;
    Input &operator=(Input &&) = delete;

    KeyboardInput m_keyboardInput;
    MouseInput m_mouseInput;
    GamepadInput m_gamepadInput;

    UnorderedMap<String, ActionState> m_actionStates;
};

inline Input *gInput = nullptr;