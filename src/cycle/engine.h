#pragma once

#include "SDL3/SDL_video.h"

#include "cycle/input/input.h"
#include "cycle/renderer.h"
#include "cycle/resource_manager.h"

class Engine
{
public:
    void init(const char *title, uint32_t width, uint32_t height);
    void shutdown();

    void run();

    bool        isRunning();
    SDL_Window *getWindow();

    ResourceManager *getResourceManager() { return &resourceManager; }
    Input           *getInput() { return &input; }

private:
    void processEvents();

    double deltaTime = 0.0f;
    bool   minimized = false;
    bool   running = false;

    Renderer        renderer;
    Camera          camera;
    Input           input;
    ResourceManager resourceManager;

    SDL_Window *window;
};