#pragma once

#include "SDL3/SDL_events.h"
#include "cycle/types/keyboard.h"
#include "cycle/types/mouse.h"
#include "cycle/math.h"

class Input
{
public:
    static void init();

    void processEvent(SDL_Event *event);

    bool isMouseButtonDown(const MouseButton &button);
    bool isMouseMoving();
    bool isKeyDown(const KeyboardKey &key);

    vec2 getMousePosition() { return mousePosition; }
    vec2 getMouseRelativePosition() { return mouseRelativePosition; }

private:
    Input() {}
    Input(Input &) = delete;
    void operator=(Input const &) = delete;

    MouseButton getMouseButtonFromSDL(uint8_t button);
    KeyboardKey getKeyFromSDL(SDL_Keycode code);

    bool keys[uint8_t(KeyboardKey::COUNT)] = {};
    bool mouse[uint8_t(MouseButton::COUNT)] = {};
    
    bool mouseMoving = false;
    vec2 mousePosition = vec3(0.0f);
    vec2 mouseRelativePosition = vec3(0.0f);
};