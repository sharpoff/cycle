#pragma once

#include <filesystem>

#include <entt/entt.hpp>

class Level
{
public:
    void load(std::filesystem::path filename);

private:
    entt::registry registry;
};