#include "core/application.h"
#include "core/filesystem.h"

int main()
{
    filesystem::setCurrentPath(filesystem::getExecutablePath().parent_path());

    Application *app = new Application("Application", 1280, 720);
    app->run();

    delete app;
    return 0;
};