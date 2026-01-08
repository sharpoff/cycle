#pragma once

#include <filesystem>

#include "cycle/types/model.h"

namespace gltf
{
    bool load(Model &model, const std::filesystem::path &filename);
}