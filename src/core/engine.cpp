#include "core/engine.h"

#include <chrono>

#include "core/asset_manager.h"
#include "core/audio.h"
#include "game/entities/static_body.h"
#include "game/world.h"
#include "input/input.h"
#include "physics/physics.h"
#include "core/logger.h"

#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>

Engine::Engine(const char *title, uint32_t width, uint32_t height)
{
    m_window.Create(title, width, height);

    gLogger = new Logger();
    gLogger->Initialize();

    gInput = new Input();
    gInput->Initialize();

    gRenderer = new Renderer();
    gRenderer->Initialize(&m_window);

    gAssetManager = new AssetManager();
    gAssetManager->Initialize();

    gPhysics = new Physics();
    gPhysics->Initialize();

    gAudio = new Audio();
    gAudio->Initialize();

    gWorld = new World();
    gWorld->Initialize();

    // create default material
    uint32_t defaultMaterialIndex = gAssetManager->CreateMaterial("default");
    Material *defaultMaterial = gAssetManager->GetMaterialByIndex(defaultMaterialIndex);
    defaultMaterial->baseColorTextureIndex = gAssetManager->CreateTexture(TexturesDir / "compressed/checkerboard.ktx", "default");
    gRenderer->AddTextureToDescriptor(defaultMaterial->baseColorTextureIndex);
    gRenderer->AddMaterialToDescriptor(defaultMaterialIndex);

    // load common textures
    gRenderer->AddTextureToDescriptor(gAssetManager->CreateTexture(TexturesDir / "sky_cubemap/sky_cubemap.ktx", "skybox"));

    // load models
    gAssetManager->CreateModel(ModelsDir / "monkey.gltf", "monkey");
    gAssetManager->CreateModel(ModelsDir / "cube.gltf", "cube");
    gAssetManager->CreateModel(ModelsDir / "sponza/Sponza.gltf", "sponza");
    // gAssetManager->CreateModel(modelsDir / "de_dust2/de_dust2.gltf", "de_dust2");
    // gAssetManager->CreateModel(modelsDir / "ak47/v_ak47.gltf", "ak47");

    // TODO: add textures/materials to descriptors using AddTextureToDescriptor, etc.

    debugCamera.SetPerspective(glm::radians(60.0f), gRenderer->GetAspectRatio(), 0.1f, 100.0f);
    debugCamera.SetPosition(vec3(0, 2, 2));
    gRenderer->SetCamera(debugCamera);

    // create entities

    StaticBody *monkeyEntity = new StaticBody();
    monkeyEntity->SetPosition(vec3(0, 0, 0));
    monkeyEntity->SetModel(gAssetManager->GetModelByName("monkey"));
    // monkeyEntity->CreateBodyFromShape(BoxShape{.halfExtents = gAssetManager->GetModel("monkey")->bounds.getHalfExtents()});
    monkeyEntity->CreateBodyFromModel();
    gPhysics->RegisterEntity(monkeyEntity);

    gWorld->AddEntity(monkeyEntity, "monkey");
}

Engine::~Engine()
{
    gWorld->Shutdown();
    gAudio->Shutdown();
    gPhysics->Shutdown();
    gAssetManager->Shutdown();
    gRenderer->Shutdown();
    gInput->Shutdown();
    gLogger->Shutdown();

    m_window.Destroy();
}

void Engine::Run()
{
    auto startTime = std::chrono::high_resolution_clock::now();

    running_ = true;
    while (running_) {
        auto endTime = std::chrono::high_resolution_clock::now();
        double deltaTime = std::chrono::duration<double, std::milli>(endTime - startTime).count() / 1000.0;
        startTime = endTime;
        time_ += deltaTime;

        ProcessEvents(deltaTime);
        Update(deltaTime);

        if (!m_window.IsMinimized()) {
            ImGui_ImplSDL3_NewFrame();
            gRenderer->RenderFrame();
        }
    }
}

void Engine::ProcessEvents(float deltaTime)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            running_ = false;
        }

        ImGui_ImplSDL3_ProcessEvent(&event);
        gInput->ProcessEvent(&event);
    }

#ifndef NDEBUG
    if (gInput->GetKeyboardInput().IsPressed(SDLK_ESCAPE))
        running_ = false;

    float camMovementSpeed = 2.0f * deltaTime;
    float camRotationSpeed = 0.3f;

    MouseInput &mouseInput = gInput->GetMouseInput();
    KeyboardInput &keyboardInput = gInput->GetKeyboardInput();

    if (mouseInput.IsPressed(SDL_BUTTON_LEFT)) {
        vec2 relPos = mouseInput.GetRelativePosition();
        debugCamera.SetRotation(debugCamera.GetRotation() + vec3(-glm::radians(relPos.y) * camRotationSpeed, glm::radians(relPos.x) * camRotationSpeed, 0.0f));
    }

    vec3 camTranslation = vec3();
    if (keyboardInput.IsPressed(SDLK_LSHIFT)) {
        camMovementSpeed *= 5.0f;
    }
    if (keyboardInput.IsPressed(SDLK_W)) {
        camTranslation.z -= camMovementSpeed;
    }
    if (keyboardInput.IsPressed(SDLK_S)) {
        camTranslation.z += camMovementSpeed;
    }
    if (keyboardInput.IsPressed(SDLK_A)) {
        camTranslation.x -= camMovementSpeed;
    }
    if (keyboardInput.IsPressed(SDLK_D)) {
        camTranslation.x += camMovementSpeed;
    }

    debugCamera.SetPosition(debugCamera.GetPosition() + (mat3(debugCamera.GetRotationMatrix()) * camTranslation));
#endif
}

void Engine::Update(float deltaTime)
{
    gInput->Update();
    gWorld->Update(deltaTime);

    gPhysics->PreUpdate();
    gPhysics->Update();
    gPhysics->PostUpdate();
}