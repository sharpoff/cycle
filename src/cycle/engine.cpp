#include "cycle/engine.h"

#include "SDL3/SDL_events.h"
#include "cycle/globals.h"
#include "cycle/input/keyboard.h"
#include "cycle/logger.h"
#include "cycle/math.h"
#include "glm/trigonometric.hpp"

#include <chrono>

#include <SDL3/SDL.h>

void Engine::init(const char *title, uint32_t width, uint32_t height)
{
    g_engine = this;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOGE("Failed to initialize SDL: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        LOGE("Failed to create SDL window: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    mat4 projection = glm::perspective(glm::radians(60.0f), float(width) / height, 0.01f, 100.0f);
    projection[1][1] *= -1;
    camera.setProjection(projection);
    camera.setPosition(vec3(0.0f, 0.0f, 1.0f));

    renderer.init(window);
    renderer.setCamera(&camera);

    input.init();
    physics.init();
    game.init();

    running = true;
}

void Engine::shutdown()
{
    game.shutdown();
    physics.shutdown();
    input.shutdown();
    renderer.shutdown();

    g_engine = nullptr;
}

void Engine::run()
{
    auto startTime = std::chrono::high_resolution_clock::now();

    while (running) {
        auto endTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::milli>(endTime - startTime).count() / 1000.0;
        startTime = endTime;

        processEvents();

        game.update();
        physics.update();

        if (!minimized) {
            renderer.draw();
        }
    }
}

void Engine::processEvents()
{
    vec3 camTranslation = vec3(0.0f);
    float movementSpeed = 2.0f * deltaTime;
    float rotationSpeed = 1.0f;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            running = false;
        }

        if (event.type == SDL_EVENT_WINDOW_MINIMIZED) {
            minimized = true;
        }

        if (event.type == SDL_EVENT_WINDOW_RESTORED) {
            minimized = false;
        }

        input.processEvent(&event);

        if (input.isMouseButtonDown(MouseButton::LEFT) && input.isMouseMoving()) {
            vec2  relPos = input.getMouseRelativePosition();
            camera.rotate(vec3(-glm::radians(relPos.y) * rotationSpeed, glm::radians(relPos.x) * rotationSpeed, 0.0f));
        }
    }

    if (input.isKeyDown(KeyboardKey::ESCAPE)) {
        running = false;
    }

    // camera movement
    if (input.isKeyDown(KeyboardKey::A)) {
        camTranslation.x -= movementSpeed;
    }
    if (input.isKeyDown(KeyboardKey::D)) {
        camTranslation.x += movementSpeed;
    }
    if (input.isKeyDown(KeyboardKey::W)) {
        camTranslation.z -= movementSpeed;
    }
    if (input.isKeyDown(KeyboardKey::S)) {
        camTranslation.z += movementSpeed;
    }
    if (input.isKeyDown(KeyboardKey::SPACE)) {
        camTranslation.y += movementSpeed;
    }

    camera.move(mat3(camera.getRotation()) * camTranslation);
}

bool Engine::isRunning()
{
    return running;
}

SDL_Window *Engine::getWindow()
{
    return window;
}