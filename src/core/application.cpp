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

    // load common textures
    gAssetManager->CreateTexture(texturesDir / "sky_cubemap/sky_cubemap.ktx", "skybox");

    // load models
    gAssetManager->CreateModel(modelsDir / "monkey.gltf", "monkey");
    gAssetManager->CreateModel(modelsDir / "cube.gltf", "cube");
    gAssetManager->CreateModel(modelsDir / "sponza/Sponza.gltf", "sponza");
    gAssetManager->CreateModel(modelsDir / "de_dust2/de_dust2.gltf", "de_dust2");
    gAssetManager->CreateModel(modelsDir / "ak47/v_ak47.gltf", "ak47");

    gRenderer->LoadDynamicResources();

    // create and add objects to the world
    gWorld = new World();
    gWorld->Init();
}

Application::~Application()
{
    gWorld->Shutdown();
    gAudio->Shutdown();
    gPhysics->Shutdown();
    gAssetManager->Shutdown();
    gRenderer->Shutdown();
    gInput->Shutdown();

    delete gWorld;
    delete gPhysics;
    delete gAudio;
    delete gRenderer;
    delete gInput;
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

float Application::GetTime()
{
    return time_;
}

vec2 Application::GetSize()
{
    return gRenderer->GetScreenSize();
}

float Application::GetAspectRatio()
{
    return gRenderer->GetAspectRatio();
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
}

void Application::Update(float deltaTime)
{
    gInput->Update();
    gWorld->Update(deltaTime);

    gPhysics->PreUpdate();
    gPhysics->Update();
    gPhysics->PostUpdate();
}