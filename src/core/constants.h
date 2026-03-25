#pragma once

#include "core/filesystem.h"

const FilePath shadersDir = "res/shaders/";
const FilePath shadersBinaryDir = "res/shaders/bin/";
const FilePath texturesDir = "res/textures/";
const FilePath modelsDir = "res/models/";
const FilePath prefabsDir = "res/prefabs/";
const FilePath audioDir = "res/audio/";
const FilePath configDir = "res/config/";

#define FRAMES_IN_FLIGHT 2

// constants below should match shaders
#define SAMPLER_LINEAR_ID 0
#define SAMPLER_NEAREST_ID 1

#define SHADOWMAP_CASCADES 4
#define SHADOWMAP_DIM 4096