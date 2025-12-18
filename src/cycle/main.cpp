#include "cycle/engine.h"
#include "cycle/filesystem.h"

int main()
{
    filesystem::setCurrentPath(filesystem::getExecutablePath().parent_path());

    Engine engine;
    engine.init("Application", 1280, 720);

    engine.run();

    engine.shutdown();
};