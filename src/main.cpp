#include "core/application.h"
#include "core/filesystem.h"

int main()
{
    SetCurrentPath(GetExecutablePath().parent_path());

    Application *app = new Application("Application", 1280, 720);
    app->Run();

    delete app;
    return 0;
};