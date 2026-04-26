#pragma once

#include "core/window.h"
#include "graphics/renderer.h"

class Engine
{
public:
    Engine(const char *title, uint32_t width, uint32_t height);
    ~Engine();

    void Run();

private:
    void ProcessEvents(float deltaTime);
    void Update(float deltaTime);

    double time_ = 0.0f;

    bool running_ = false;

    Window m_window;
    Camera debugCamera;
};