#include "cycle/engine.h"

#include "SDL3/SDL_events.h"
#include "cycle/globals.h"
#include "cycle/logger.h"
#include "cycle/math.h"
#include "glm/trigonometric.hpp"

#include <chrono>

#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"

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

    Input::init();
    GamepadInput::init();
    EntityManager::init();

    camera.setPerspective(glm::radians(60.0f), float(width) / height, 0.01f);
    camera.setPosition(vec3(0.0f, 0.0f, 1.0f));

    renderer.init(window);

    renderer.setCamera(&camera);
    editor.setCamera(&camera);

    // load models
    {
        ModelID modelID = ModelID::Invalid;
        // modelID = g_modelManager->loadModel(modelsDir / "sponza/Sponza.gltf", "sponza");
        modelID = g_modelManager->loadModel(modelsDir / "monkey.gltf", "monkey");
        modelID = g_modelManager->loadModel(modelsDir / "cube.gltf", "cube");
    }

    // load textures
    {
        TextureID texID = TextureID::Invalid;
        texID = g_textureManager->createTextureCubeMap("resources/textures/sky_cubemap/", "skybox");
    }

    createEntities();

    // should be called *after* loading entities/materials/textures/models/etc.
    renderer.loadDynamicResources();

    // should be called *after* create entities, to affect physics entity components
    physics.init();
}

void Engine::shutdown()
{
    physics.shutdown();
    renderer.shutdown();

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
            renderer.draw(editor);
        }
    }
}

void Engine::createEntities()
{
    // sponza
    // {
    //     const EntityID      sponza = g_entityManager->createEntity();
    //     TransformComponent &transform = g_entityManager->transformComponents.addComponent(sponza);
    //     transform.transform = glm::scale(vec3(0.01f));

    //     NameComponent &nameComponent = g_entityManager->nameComponents.addComponent(sponza);
    //     nameComponent.name = "sponza";

    //     RenderComponent &renderComponent = g_entityManager->renderComponents.addComponent(sponza);
    //     renderComponent.modelID = g_modelManager->getModelIDByName("sponza");
    // }

    // monkey
    {
        const EntityID monkey = g_entityManager->createEntity();
        TransformComponent &transform = g_entityManager->transformComponents.addComponent(monkey);
        transform.transform = glm::translate(vec3(0.0f, 100.0f, 0.0f));

        NameComponent &nameComponent = g_entityManager->nameComponents.addComponent(monkey);
        nameComponent.name = "monkey";

        RenderComponent &renderComponent = g_entityManager->renderComponents.addComponent(monkey);
        renderComponent.modelID = g_modelManager->getModelIDByName("monkey");

        RigidBodyComponent &rigidbodyComponent = g_entityManager->rigidBodyComponents.addComponent(monkey);
        rigidbodyComponent.isDynamic = true;
        rigidbodyComponent.type = RigidBodyType::Box;
        rigidbodyComponent.halfExtent = g_modelManager->getHalfExtent(renderComponent.modelID);
    }

    // floor
    {
        const EntityID box = g_entityManager->createEntity();
        TransformComponent &transform = g_entityManager->transformComponents.addComponent(box);
        transform.transform = glm::scale(vec3(10.0f, 0.1f, 10.0f));

        NameComponent &nameComponent = g_entityManager->nameComponents.addComponent(box);
        nameComponent.name = "floor";

        RenderComponent &renderComponent = g_entityManager->renderComponents.addComponent(box);
        renderComponent.modelID = g_modelManager->getModelIDByName("cube");

        RigidBodyComponent &rigidbodyComponent = g_entityManager->rigidBodyComponents.addComponent(box);
        rigidbodyComponent.isDynamic = false;
        rigidbodyComponent.type = RigidBodyType::Box;
        rigidbodyComponent.halfExtent = g_modelManager->getHalfExtent(renderComponent.modelID, math::getScale(transform.transform));
    }

    // light
    {
        const EntityID light = g_entityManager->createEntity();
        LightComponent &lightComponent = g_entityManager->lightComponents.addComponent(light);
        lightComponent.lightType = LIGHT_TYPE_DIRECTIONAL;
        lightComponent.color = vec3(1.0f);

        NameComponent &nameComponent = g_entityManager->nameComponents.addComponent(light);
        nameComponent.name = "light";

        RenderComponent &renderComponent = g_entityManager->renderComponents.addComponent(light);
        renderComponent.modelID = g_modelManager->getModelIDByName("cube");

        TransformComponent &transformComponent = g_entityManager->transformComponents.addComponent(light);
        transformComponent.transform = glm::translate(vec3(0.0f, 5.0f, 0.0f)) * glm::scale(vec3(0.2f));
    }
}

void Engine::processEvents()
{
    vec3 camTranslation = vec3(0.0f);
    float camMovementSpeed = 2.0f * deltaTime;
    float camRotationSpeed = 1.0f;
    float camRotationSpeedGamepad = 0.2f;

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
        renderer.reloadShaders();
    }

    editor.processInput();

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
    for (const EntityID entity : g_entityManager->nameComponents.getEntities()) {
        NameComponent *nameComp = g_entityManager->nameComponents.getComponent(entity);
        if (nameComp && !strcmp(nameComp->name, "light")) {
            TransformComponent *transformComp = g_entityManager->transformComponents.getComponent(entity);
            if (!transformComp)
                continue;

            const mat4 &transform = transformComp->transform;

            vec3 pos = math::getPosition(transform);
            pos.x = sin(glm::radians(time) * 360.0f * 0.2f) * 5.0f;
            pos.z = cos(glm::radians(time) * 360.0f * 0.2f) * 5.0f;
            transformComp->transform = glm::translate(pos) * mat4(math::getRotation(transform)) * glm::scale(math::getScale(transform));
        }
    }

    physics.update();
}