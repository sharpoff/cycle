#pragma once

#include "core/containers.h"

const FilePath ShadersDir = "res/shaders/";
const FilePath ShadersBinaryDir = "res/shaders/bin/";
const FilePath TexturesDir = "res/textures/";
const FilePath ModelsDir = "res/models/";
const FilePath PrefabsDir = "res/prefabs/";
const FilePath AudioDir = "res/audio/";
const FilePath ConfigDir = "res/config/";
const FilePath LogsDir = "res/logs/";

static const char *ImGuiConfigFile = "assets/config/imgui.ini";
static const char *ImGuiLogFile = "assets/logs/imgui_log.txt";

#define FRAMES_IN_FLIGHT 2

// constants below should match shaders
#define SAMPLER_LINEAR_ID 0
#define SAMPLER_NEAREST_ID 1

#define SHADOWMAP_CASCADES 4
#define SHADOWMAP_DIM 4096

const constexpr uint32_t InvalidIndex = UINT32_MAX;