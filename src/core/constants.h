#pragma once

#include <filesystem>

const std::filesystem::path shadersDir = "assets/shaders/";
const std::filesystem::path shadersBinaryDir = "assets/shaders/bin/";
const std::filesystem::path texturesDir = "assets/textures/";
const std::filesystem::path modelsDir = "assets/models/";
const std::filesystem::path prefabsDir = "assets/prefabs/";
const std::filesystem::path audioDir = "assets/audio/";
const std::filesystem::path configDir = "assets/config/";

const int FRAMES_IN_FLIGHT = 2;

// constants below should match shaders
const int SAMPLER_LINEAR_ID = 0;
const int SAMPLER_NEAREST_ID = 1;

const int SHADOWMAP_CASCADES = 4;
const int SHADOWMAP_DIM = 4096;