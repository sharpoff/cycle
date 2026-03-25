#pragma once

#include "graphics/vulkan_types.h"
#include <memory>

using TexturePtr = std::shared_ptr<Texture>;

struct Material
{
    TexturePtr baseColorTex = nullptr;
    TexturePtr metallicRoughnessTex = nullptr;
    TexturePtr normalTex = nullptr;
    TexturePtr emissiveTex = nullptr;
    float roughnessFactor = 0.0f;
    float metallicFactor = 0.0f;
};