#include "core/engine.h"
#include "core/filesystem.h"

int main()
{
    SetCurrentPath(GetExecutablePath().parent_path());

    Engine *app = new Engine("Application", 1280, 720);
    app->Run();

    delete app;
    return 0;
};