#pragma once

#include "SDL3/SDL_video.h"

#include "cycle/input/input.h"
#include "cycle/renderer.h"
#include "cycle/resource_manager.h"
#include "cycle/types/scene.h"

class Engine
{
public:
    void init(const char *title, uint32_t width, uint32_t height);
    void shutdown();

    void run();

    ResourceManager &getResourceManager() { return resourceManager; }
    Scene           &getScene() { return scene; }

    bool isRunning() { return running; }

private:
    void processEvents();

    double deltaTime = 0.0f;
    bool   minimized = false;
    bool   running = false;

    Renderer        renderer;
    ResourceManager resourceManager;
    Camera          camera;
    Input           input;
    Scene           scene;

    SDL_Window *window;
};