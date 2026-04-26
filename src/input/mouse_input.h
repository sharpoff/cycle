#pragma once

#include "SDL3/SDL_events.h"
#include "core/containers.h"
#include "input/action_state.h"
#include "math/math_types.h"

class MouseInput
{
public:
    void Update();
    void ProcessEvent(SDL_Event *event);

    void RegisterAction(const String &actionName, SDL_MouseButtonFlags scancode);

    bool WasJustPressed(SDL_MouseButtonFlags scancode);
    bool IsPressed(SDL_MouseButtonFlags scancode);
    bool IsReleased(SDL_MouseButtonFlags scancode);

    bool IsMoving() { return m_moving; }

    vec2 &GetRelativePosition() { return m_position; }

private:
    struct MouseState
    {
        bool current = false;
        bool previous = false;
    };

    UnorderedMap<SDL_MouseButtonFlags, MouseState> m_buttons{};
    vec2 m_position = vec2(0.0f);

    bool m_moving = false;

    UnorderedMap<String, Vector<SDL_MouseButtonFlags>> m_mouseActions;
    UnorderedMap<SDL_MouseButtonFlags, ActionState> m_actionStates;
};