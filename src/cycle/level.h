#pragma once

#include "cycle/containers.h"
#include "cycle/math.h"
#include <filesystem>

class Level
{
public:
    void loadPrefab(std::filesystem::path path, String name = "", vec3 position = vec3(0.0f));
private:
};