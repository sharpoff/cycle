#pragma once

#include "cycle/types/id.h"
#include <filesystem>

class Level
{
public:
    const EntityID loadPrefab(std::filesystem::path path);
private:
};