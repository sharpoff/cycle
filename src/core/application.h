#pragma once

#include "SDL3/SDL_video.h"
#include "graphics/renderer.h"

class Application
{
public:
    Application(const char *title, uint32_t width, uint32_t height);
    ~Application();

    void Run();

    float GetTime();
    vec2  GetSize();
    float GetAspectRatio();

    bool IsMinimized() { return minimized_; }
    bool IsRunning() { return running_; }

private:
    void ProcessEvents(float deltaTime);
    void Update(float deltaTime);

    double time_ = 0.0f;

    bool minimized_ = false;
    bool running_ = false;

    SDL_Window *window_;
};