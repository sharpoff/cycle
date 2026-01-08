#include "cycle/engine.h"

#include <chrono>

#include "cycle/audio.h"
#include "cycle/logger.h"
#include "cycle/math.h"
#include "cycle/editor.h"
#include "cycle/physics.h"
#include "cycle/renderer.h"
#include "cycle/input.h"
#include "cycle/gamepad_input.h"

#include "cycle/managers/entity_manager.h"
#include "cycle/managers/model_manager.h"
#include "cycle/managers/texture_manager.h"

#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"

extern Input *g_input;
extern Physics *g_physics;
extern GamepadInput *g_gamepadInput;
extern EntityManager *g_entityManager;
extern ModelManager *g_modelManager;
extern TextureManager *g_textureManager;
extern Editor *g_editor;
extern Audio *g_audio;
extern Renderer *g_renderer;

Engine *g_engine;

void Engine::init(const char *title, uint32_t width, uint32_t height)
{
    g_engine = this;

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

    g_renderer->init(window);
    g_input->init();
    g_gamepadInput->init();
    g_physics->init();
    g_audio->init();
    g_editor->init();
    g_entityManager->init();

    camera.setPerspective(glm::radians(60.0f), float(width) / height, 0.01f);
    camera.setPosition(vec3(0.0f, 0.0f, 1.0f));
    g_renderer->setCamera(&camera);
    g_editor->setCamera(&camera);

    // load common textures
    {
        TextureID texID = TextureID::Invalid;
        texID = g_textureManager->createTexture("resources/textures/sky_cubemap/sky_cubemap.ktx", "skybox");
    }

    // load common models
    {
        ModelID modelID = ModelID::Invalid;
        modelID = g_modelManager->loadModel(modelsDir / "monkey.gltf", "monkey");
        modelID = g_modelManager->loadModel(modelsDir / "cube.gltf", "cube");
    }

    level.loadPrefab(prefabsDir / "cube.json");
    level.loadPrefab(prefabsDir / "sponza.json");
    level.loadPrefab(prefabsDir / "light.json");

    g_renderer->loadDynamicResources();
    g_physics->createBodies();

    g_entityManager->destroyEntity(g_entityManager->getEntityIDByName("box"));

    // g_audio->load(audioDir / "ambient_drums.wav", "ambient_drums");
    // g_audio->play("ambient_drums", true);
}

void Engine::shutdown()
{
    g_physics->shutdown();
    g_audio->shutdown();
    g_renderer->shutdown();

    g_engine = nullptr;
}

void Engine::run()
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
            g_renderer->draw();
        }
    }
}

void Engine::processEvents()
{
    vec3 camTranslation = vec3(0.0f);
    float camMovementSpeed = 2.0f * deltaTime;
    float camRotationSpeed = 1.0f;
    float camRotationSpeedGamepad = 0.2f;

    bool cameraMoved = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            running = false;
        }

        if (event.type == SDL_EVENT_WINDOW_MINIMIZED) {
            minimized = true;
        }

        if (event.type == SDL_EVENT_WINDOW_RESTORED) {
            minimized = false;
        }

        g_input->processEvent(&event);
        g_gamepadInput->processEvent(&event);

        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse)
            continue; // skip mouse handling

        // gamepad right axis movement
        if (g_gamepadInput->isConnected()) {
            const GamepadState &gamepadState = g_gamepadInput->getGamepadState();
            vec2 relPos = vec2(0.0f);
            if (abs(gamepadState.rightAxisX) > gamepadState.deadZone) {
                relPos.x = gamepadState.rightAxisX / 32767.0f;
            }
            if (abs(gamepadState.rightAxisY) > gamepadState.deadZone) {
                relPos.y = gamepadState.rightAxisY / 32767.0f;
            }

            camera.rotate(vec3(-glm::radians(relPos.y) * camRotationSpeedGamepad, glm::radians(relPos.x) * camRotationSpeedGamepad, 0.0f));
        }

        // mouse movement
        if (g_input->isMouseButtonDown(MouseButton::LEFT) && g_input->isMouseMoving()) {
            vec2 relPos = g_input->getMouseRelativePosition();
            camera.rotate(vec3(-glm::radians(relPos.y) * camRotationSpeed, glm::radians(relPos.x) * camRotationSpeed, 0.0f));
        }
    }

    if (g_input->isKeyDown(KeyboardKey::ESCAPE)) {
        running = false;
    }

    if (g_input->isKeyDown(KeyboardKey::P)) {
        LOGI("%s", "Reloading shaders");
        g_renderer->reloadShaders();
    }

    g_editor->processInput();

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
        return; // skip keyboard handling

    // gamepad left axis movement
    if (g_gamepadInput->isConnected()) {
        const GamepadState &gamepadState = g_gamepadInput->getGamepadState();
        if (abs(gamepadState.leftAxisX) > gamepadState.deadZone) {
            if (gamepadState.leftAxisX > 0)
                camTranslation.x += camMovementSpeed;
            else
                camTranslation.x -= camMovementSpeed;
        }

        if (abs(gamepadState.leftAxisY) > gamepadState.deadZone) {
            if (gamepadState.leftAxisY > 0)
                camTranslation.z += camMovementSpeed;
            else
                camTranslation.z -= camMovementSpeed;
        }
    }

    // camera movement
    if (g_input->isKeyDown(KeyboardKey::LSHIFT)) {
        camMovementSpeed *= 5;
    }
    if (g_input->isKeyDown(KeyboardKey::A)) {
        camTranslation.x -= camMovementSpeed;
    }
    if (g_input->isKeyDown(KeyboardKey::D)) {
        camTranslation.x += camMovementSpeed;
    }
    if (g_input->isKeyDown(KeyboardKey::W)) {
        camTranslation.z -= camMovementSpeed;
    }
    if (g_input->isKeyDown(KeyboardKey::S)) {
        camTranslation.z += camMovementSpeed;
    }
    if (g_input->isKeyDown(KeyboardKey::SPACE)) {
        camTranslation.y += camMovementSpeed;
    }

    camera.move(mat3(camera.getRotation()) * camTranslation);
}

void Engine::update()
{
    g_physics->update();
}