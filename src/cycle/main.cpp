#include "cycle/engine.h"
#include "cycle/filesystem.h"
#include "cycle/input.h"
#include "cycle/game/game.h"
#include "cycle/physics/physics.h"
#include "cycle/renderer.h"

int main()
{
    filesystem::setCurrentPath(filesystem::getExecutablePath().parent_path());

    Engine::Init("Application", 1280, 720);

    while (Engine::IsRunning()) {
        Engine::Update();
        Input::Process();
        Game::Update();
        Physics::Update();
        Renderer::Draw();
    }

    Engine::Shutdown();
};