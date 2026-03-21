#pragma once

#include "graphics/id.h"

struct Material
{
    TextureID baseColorTexID = TextureID::Invalid;
    TextureID metallicRoughnessTexID = TextureID::Invalid;
    TextureID normalTexID = TextureID::Invalid;
    TextureID emissiveTexID = TextureID::Invalid;
    float roughnessFactor = 0.0f;
    float metallicFactor = 0.0f;
};