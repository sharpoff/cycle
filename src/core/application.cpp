#include "core/application.h"

#include <chrono>

#include "core/logger.h"
#include "math/math_types.h"

#include "graphics/cache/texture_cache.h"
#include "gltf_loader.h"
#include "core/logger.h"

#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"

Application::Application(const char *title, uint32_t width, uint32_t height)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        LOGE("Failed to initialize SDL: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        LOGE("Failed to create SDL window: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RaiseWindow(window);

    renderer = std::make_unique<Renderer>(window);
    renderer->init();

    physics = std::make_unique<PhysicsSystem>();
    physics->init();

    audio = std::make_unique<AudioSystem>();
    audio->init();

    camera.setPerspective(glm::radians(60.0f), float(width) / height, 0.01f, 1000.0f);
    camera.setPosition(vec3(0.0f, 0.0f, 1.0f));
    camera.setRotation(vec3(glm::radians(10.0f), 0.0f, 0.0f));

    // load common textures
    auto &cacheMgr = renderer->getCacheManager();
    cacheMgr.getTextureCache().loadTexture(texturesDir / "sky_cubemap/sky_cubemap.ktx", VK_FORMAT_R8G8B8A8_SRGB, "skybox");

    // load models
    if (!GLTFLoader::load(cacheMgr, scene, modelsDir / "monkey.gltf") ||
        !GLTFLoader::load(cacheMgr, scene, modelsDir / "cube.gltf") ||
        !GLTFLoader::load(cacheMgr, scene, modelsDir / "sponza/Sponza.gltf") ||
        !GLTFLoader::load(cacheMgr, scene, modelsDir / "de_dust2/de_dust2.gltf")
    ) {
        LOGE("Failed to load gltf models!", NULL);
    }

    renderer->loadDynamicResources();
    physics->createBodies();
}

Application::~Application()
{
    physics->shutdown();
    audio->shutdown();
    renderer->shutdown();
}

void Application::run()
{
    auto startTime = std::chrono::high_resolution_clock::now();

    running = true;
    while (running) {
        auto endTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::milli>(endTime - startTime).count() / 1000.0;
        startTime = endTime;
        time += deltaTime;

        processEvents();
        update();

        if (!minimized) {
            ImGui_ImplSDL3_NewFrame();
            renderer->renderFrame(camera);
        }
    }
}

void Application::processEvents()
{
    vec3 camTranslation = vec3(0.0f);
    float camMovementSpeed = 2.0f * deltaTime;
    float camRotationSpeed = 0.2f;
    float camRotationSpeedGamepad = 0.2f;

    bool cameraMoved = false;

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

        ImGui_ImplSDL3_ProcessEvent(&event);
        keyboardInput.processEvent(&event);
        mouseInput.processEvent(&event);

        ImGuiIO &io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            // mouse movement
            if (mouseInput.isPressed(SDL_BUTTON_LEFT) && event.type == SDL_EVENT_MOUSE_MOTION) {
                camera.rotate(vec3(-glm::radians(event.motion.yrel) * camRotationSpeed, glm::radians(event.motion.xrel) * camRotationSpeed, 0.0f));
            }
        }
    }

    if (keyboardInput.isPressed(SDLK_ESCAPE)) {
        running = false;
    }

    if (keyboardInput.isPressed(SDLK_P)) {
        LOGI("%s", "Reloading shaders");
        renderer->reloadShaders();
    }

    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        // camera movement
        if (keyboardInput.isPressed(SDLK_LSHIFT)) {
            camMovementSpeed *= 5;
        }
        if (keyboardInput.isPressed(SDLK_A)) {
            camTranslation.x -= camMovementSpeed;
        }
        if (keyboardInput.isPressed(SDLK_D)) {
            camTranslation.x += camMovementSpeed;
        }
        if (keyboardInput.isPressed(SDLK_W)) {
            camTranslation.z -= camMovementSpeed;
        }
        if (keyboardInput.isPressed(SDLK_S)) {
            camTranslation.z += camMovementSpeed;
        }
        if (keyboardInput.isPressed(SDLK_SPACE)) {
            camTranslation.y += camMovementSpeed;
        }
    }

    // // gamepad left axis movement
    // if (gamepadInput.isConnected()) {
    //     const GamepadState &gamepadState = gamepadInput.getGamepadState();
    //     if (abs(gamepadState.leftAxisX) > gamepadState.deadZone) {
    //         if (gamepadState.leftAxisX > 0)
    //             camTranslation.x += camMovementSpeed;
    //         else
    //             camTranslation.x -= camMovementSpeed;
    //     }

    //     if (abs(gamepadState.leftAxisY) > gamepadState.deadZone) {
    //         if (gamepadState.leftAxisY > 0)
    //             camTranslation.z += camMovementSpeed;
    //         else
    //             camTranslation.z -= camMovementSpeed;
    //     }
    // }

    camera.move(mat3(camera.getRotation()) * camTranslation);
}

void Application::update()
{
    physics->update();
}