#pragma once

#include <filesystem>

const std::filesystem::path shadersDir = "resources/shaders/";
const std::filesystem::path shadersBinaryDir = "resources/shaders/bin/";
const std::filesystem::path texturesDir = "resources/textures/";
const std::filesystem::path modelsDir = "resources/models/";

#include "cycle/engine.h"

inline Engine *g_engine = nullptr;