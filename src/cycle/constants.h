#pragma once

#include <filesystem>

const std::filesystem::path shadersDir = "resources/shaders/";
const std::filesystem::path shadersBinaryDir = "resources/shaders/bin/";
const std::filesystem::path texturesDir = "resources/textures/";
const std::filesystem::path modelsDir = "resources/models/";
const std::filesystem::path prefabsDir = "resources/prefabs/";

const int FRAMES_IN_FLIGHT = 2;

// constants below should match shaders
const int SAMPLER_LINEAR_ID = 0;
const int SAMPLER_NEAREST_ID = 1;

const int SHADOWMAP_CASCADES = 4;