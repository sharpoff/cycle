#pragma once

#include "SDL3/SDL_video.h"
#include "cycle/types/camera.h"

class Engine
{
public:
    void init(const char *title, uint32_t width, uint32_t height);
    void shutdown();

    void run();

    bool isRunning() { return running; }

    double getDeltaTime() { return deltaTime; }
    double getTime() { return time; }

private:
    void createEntities();
    void processEvents();
    void update();

    double deltaTime = 0.0f;
    double time = 0.0f;
    bool   minimized = false;
    bool   running = false;

    Camera camera;

    SDL_Window *window;
};