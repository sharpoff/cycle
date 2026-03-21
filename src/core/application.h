#pragma once

#include "SDL3/SDL_video.h"
#include "core/audio.h"
#include "core/world.h"
#include "input/keyboard_input.h"
#include "input/mouse_input.h"
#include "physics/physics.h"
#include "graphics/renderer.h"
#include "core/camera.h"

class Application
{
public:
    Application(const char *title, uint32_t width, uint32_t height);
    ~Application();

    void run();

    bool isRunning() { return running; }

    double getDeltaTime() { return deltaTime; }
    double getTime() { return time; }

private:
    void processEvents();
    void update();

    double deltaTime = 0.0f;
    double time = 0.0f;

    bool minimized = false;
    bool running = false;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<PhysicsSystem> physics;
    std::unique_ptr<AudioSystem> audio;

    KeyboardInput keyboardInput;
    MouseInput mouseInput;
    Camera camera;
    World world;

    SDL_Window *window;
};