#include "core/application.h"

#include <chrono>

#include "core/asset_manager.h"
#include "core/audio.h"
#include "core/logger.h"
#include "game/world.h"
#include "input/input_manager.h"
#include "physics/physics.h"

#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>

Application::Application(const char *title, uint32_t width, uint32_t height)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        LOGE("Failed to initialize SDL: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    window_ = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        LOGE("Failed to create SDL window: %s", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_RaiseWindow(window_);

    gInput = new InputManager();
    gInput->Init();

    gRenderer = new Renderer();
    gRenderer->Init(window_);

    gAssetManager = new AssetManager();
    gAssetManager->Init();

    gPhysics = new Physics();
    gPhysics->Init();

    gAudio = new Audio();
    gAudio->Init();

    // create default material
    Material *defaultMaterial = gAssetManager->CreateMaterial("default");
    defaultMaterial->baseColorTex = gAssetManager->CreateTexture(texturesDir / "compressed/checkerboard.ktx", "default");
    gRenderer->AddTextureToDescriptor(defaultMaterial->baseColorTex);
    gRenderer->AddMaterialToDescriptor(defaultMaterial);

    // load common textures
    gRenderer->AddTextureToDescriptor(gAssetManager->CreateTexture(texturesDir / "sky_cubemap/sky_cubemap.ktx", "skybox"));

    // load models
    gAssetManager->CreateModel(modelsDir / "monkey.gltf", "monkey");
    gAssetManager->CreateModel(modelsDir / "cube.gltf", "cube");
    gAssetManager->CreateModel(modelsDir / "sponza/Sponza.gltf", "sponza");
    // gAssetManager->CreateModel(modelsDir / "de_dust2/de_dust2.gltf", "de_dust2");
    // gAssetManager->CreateModel(modelsDir / "ak47/v_ak47.gltf", "ak47");

    // TODO: add textures/materials to descriptors using AddTextureToDescriptor, etc.

    gRenderer->LoadDynamicResources();

    debugCamera.SetPerspective(glm::radians(60.0f), gRenderer->GetAspectRatio(), 0.1f, 100.0f);
    debugCamera.SetPosition(vec3(0, 2, 2));
    gRenderer->SetCamera(debugCamera);

    gWorld = new World();
    gWorld->Init();

    Entity *entity = new Entity();
    entity->SetModel(gAssetManager->GetModel("monkey"));
    gWorld->AddEntity(entity, "entity");
}

Application::~Application()
{
    gWorld->Shutdown();
    gAudio->Shutdown();
    gPhysics->Shutdown();

    // destroy textures
    auto &textures = gAssetManager->GetTextures();
    for (auto &texture : textures) {
        gRenderer->GetDevice().DestroyTexture(texture);
    }

    // destroy mesh buffers
    auto &models = gAssetManager->GetModels();
    for (auto &model : models) {
        for (auto &mesh : model->meshes) {
            for (auto &prim : mesh.primitives) {
                gRenderer->GetDevice().DestroyBuffer(prim.vertexBuffer);
                gRenderer->GetDevice().DestroyBuffer(prim.indexBuffer);
            }
        }
    }

    gAssetManager->Shutdown();
    gRenderer->Shutdown();
    gInput->Shutdown();
}

void Application::Run()
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

        if (!minimized_) {
            ImGui_ImplSDL3_NewFrame();
            gRenderer->DrawFrame();
        }
    }
}

void Application::ProcessEvents(float deltaTime)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            running_ = false;
        }

        ImGui_ImplSDL3_ProcessEvent(&event);
        gInput->ProcessEvent(&event);
    }

    minimized_ = false;
    if (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED) {
        minimized_ = true;
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

void Application::Update(float deltaTime)
{
    gInput->Update();
    gWorld->Update(deltaTime);

    gPhysics->PreUpdate();
    gPhysics->Update();
    gPhysics->PostUpdate();
}