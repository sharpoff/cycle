#include "core/application.h"

#include <chrono>

#include "core/logger.h"
#include "math/math_types.h"

#include "graphics/cache/texture_cache.h"
#include "core/gltf_loader.h"
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

    CacheManager &cacheMgr = renderer->getCacheManager();

    // load common textures
    TextureCache &textureCache = cacheMgr.getTextureCache();
    textureCache.loadFromFile(texturesDir / "sky_cubemap/sky_cubemap.ktx", VK_FORMAT_R8G8B8A8_SRGB, "skybox");

    // load models
    ModelCache &modelCache = cacheMgr.getModelCache();
    auto monkeyModelID = modelCache.loadFromFile(modelsDir / "monkey.gltf", "monkey");
    modelCache.loadFromFile(modelsDir / "sponza/Sponza.gltf", "sponza");
    modelCache.loadFromFile(modelsDir / "de_dust2/de_dust2.gltf", "de_dust2");
    modelCache.loadFromFile(modelsDir / "ak47/v_ak47.gltf", "ak47");

    Object *monkey = new Object();
    monkey->setModelID(monkeyModelID);
    world.addObject(monkey, "monkey");

    renderer->loadDynamicResources();
}

Application::~Application()
{
    world.freeObject("monkey");
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
            renderer->drawFrame(world, camera);
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

    camera.move(mat3(camera.getRotation()) * camTranslation);
}

void Application::update()
{
    physics->update();
}