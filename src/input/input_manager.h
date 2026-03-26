#pragma once

#include "input/gamepad_input.h"
#include "input/keyboard_input.h"
#include "input/mouse_input.h"

class InputManager
{
public:
    friend class Application;

    void Init();
    void Shutdown();

    void Update();
    void ProcessEvent(SDL_Event *event);

    KeyboardInput &GetKeyboardInput() { return keyboardInput_; }
    MouseInput &GetMouseInput() { return mouseInput_; }
    GamepadInput &GetGamepadInput() { return gamepadInput_; }

private:
    InputManager() {}
    InputManager(const InputManager &) = delete;
    InputManager(InputManager &&) = delete;
    InputManager &operator=(const InputManager &) = delete;
    InputManager &operator=(InputManager &&) = delete;

    KeyboardInput keyboardInput_;
    MouseInput mouseInput_;
    GamepadInput gamepadInput_;
};

inline InputManager *gInput = nullptr;