#pragma once

struct ActionState
{
    bool wasJustPressed = false;
    bool isPressed = false;
    bool isReleased = false;
    float axisState = 0.0f; // gamepad only
};