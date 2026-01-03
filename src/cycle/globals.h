#pragma once

#include <filesystem>

#include "cycle/engine.h"
#include "cycle/input/gamepad_input.h"
#include "cycle/input/input.h"
#include "cycle/managers/material_manager.h"
#include "cycle/managers/mesh_manager.h"
#include "cycle/managers/model_manager.h"
#include "cycle/managers/texture_manager.h"

const std::filesystem::path shadersDir = "resources/shaders/";
const std::filesystem::path shadersBinaryDir = "resources/shaders/bin/";
const std::filesystem::path texturesDir = "resources/textures/";
const std::filesystem::path modelsDir = "resources/models/";

inline Engine *g_engine = nullptr;
inline Input *g_input = nullptr;
inline GamepadInput *g_gamepadInput = nullptr;

inline TextureManager *g_textureManager = nullptr;
inline MeshManager *g_meshManager = nullptr;
inline MaterialManager *g_materialManager = nullptr;
inline ModelManager *g_modelManager = nullptr;