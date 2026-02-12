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

    Renderer::init(window);
    Input::init();
    GamepadInput::init();
    Physics::init();
    Audio::init();
    Editor::init();
    EntityManager::init();

    camera.setPerspective(glm::radians(60.0f), float(width) / height, 0.01f, 1000.0f);
    camera.setPosition(vec3(0.0f, 0.0f, 1.0f));
    camera.setRotation(vec3(glm::radians(10.0f), 0.0f, 0.0f));
    Renderer::get()->setCamera(&camera);
    Editor::get()->setCamera(&camera);

    // load common textures
    {
        TextureID texID = TextureID::Invalid;
        texID = TextureManager::get()->createTexture("resources/textures/sky_cubemap/sky_cubemap.ktx", VK_FORMAT_R8G8B8A8_SRGB, "skybox");
    }

    // load common models
    {
        ModelID modelID = ModelID::Invalid;
        modelID = ModelManager::get()->loadModel(modelsDir / "monkey.gltf", "monkey");
        modelID = ModelManager::get()->loadModel(modelsDir / "cube.gltf", "cube");
    }

    level.loadPrefab(prefabsDir / "light.json");

    level.loadPrefab(prefabsDir / "sponza.json");

    // {
    //     auto helmet = level.loadPrefab(prefabsDir / "helmet.json");
    //     TransformComponent *transform = g_entityManager->transforms.getComponent(helmet);
    //     if (transform)
    //         transform->transform = glm::rotate(glm::radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    // }

    // level.loadPrefab(prefabsDir / "cube.json");

    // {
    //     auto ground = level.loadPrefab(prefabsDir / "ground.json");
    //     TransformComponent *transform = g_entityManager->transforms.getComponent(ground);
    //     if (transform)
    //         transform->transform *= glm::translate(vec3(0.0f, -10.0f, 0.0f));
    // }

    // for (int i = 0; i < 3; i++) {
    //     auto monkey = level.loadPrefab(prefabsDir / "monkey.json");
    //     TransformComponent *transform = g_entityManager->transforms.getComponent(monkey);
    //     if (transform)
    //         transform->transform *= glm::translate(vec3(0.0f, 20.0f * i, 0.0f));

    //     NameComponent *name = g_entityManager->names.getComponent(monkey);
    //     if (name)
    //         name->name = "monkey" + std::to_string(i);
    // }

    Renderer::get()->loadDynamicResources();
    Physics::get()->createBodies();

    EntityManager::get()->destroyEntity(EntityManager::get()->getEntityIDByName("box"));

    // g_audio->load(audioDir / "ambient_drums.wav", "ambient_drums");
    // g_audio->play("ambient_drums", true);
}

void Engine::shutdown()
{
    Physics::get()->shutdown();
    Audio::get()->shutdown();
    Renderer::get()->shutdown();

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
            Renderer::get()->draw();
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

        Input::get()->processEvent(&event);
        GamepadInput::get()->processEvent(&event);

        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse)
            continue; // skip mouse handling

        // gamepad right axis movement
        if (GamepadInput::get()->isConnected()) {
            const GamepadState &gamepadState = GamepadInput::get()->getGamepadState();
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
        if (Input::get()->isMouseButtonDown(MouseButton::LEFT) && Input::get()->isMouseMoving()) {
            vec2 relPos = Input::get()->getMouseRelativePosition();
            camera.rotate(vec3(-glm::radians(relPos.y) * camRotationSpeed, glm::radians(relPos.x) * camRotationSpeed, 0.0f));
        }
    }

    if (Input::get()->isKeyDown(KeyboardKey::ESCAPE)) {
        running = false;
    }

    if (Input::get()->isKeyDown(KeyboardKey::P)) {
        LOGI("%s", "Reloading shaders");
        Renderer::get()->reloadShaders();
    }

    Editor::get()->processInput();

    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
        return; // skip keyboard handling

    // gamepad left axis movement
    if (GamepadInput::get()->isConnected()) {
        const GamepadState &gamepadState = GamepadInput::get()->getGamepadState();
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
    if (Input::get()->isKeyDown(KeyboardKey::LSHIFT)) {
        camMovementSpeed *= 5;
    }
    if (Input::get()->isKeyDown(KeyboardKey::A)) {
        camTranslation.x -= camMovementSpeed;
    }
    if (Input::get()->isKeyDown(KeyboardKey::D)) {
        camTranslation.x += camMovementSpeed;
    }
    if (Input::get()->isKeyDown(KeyboardKey::W)) {
        camTranslation.z -= camMovementSpeed;
    }
    if (Input::get()->isKeyDown(KeyboardKey::S)) {
        camTranslation.z += camMovementSpeed;
    }
    if (Input::get()->isKeyDown(KeyboardKey::SPACE)) {
        camTranslation.y += camMovementSpeed;
    }

    camera.move(mat3(camera.getRotation()) * camTranslation);
}

void Engine::update()
{
    Physics::get()->update();
}