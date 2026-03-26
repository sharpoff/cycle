#pragma once

#include "graphics/vulkan_types.h"

struct Material
{
public:
    Texture * baseColorTex = nullptr;
    Texture * metallicRoughnessTex = nullptr;
    Texture * normalTex = nullptr;
    Texture * emissiveTex = nullptr;
    float roughnessFactor = 0.0f;
    float metallicFactor = 0.0f;

    void SetID(uint32_t id) { this->id = id; }
    uint32_t GetID() { return id; }

private:
    uint32_t id = UINT32_MAX;
};

struct GPUMaterial
{
    uint32_t baseColorTexID = UINT32_MAX;
    uint32_t metallicRoughnessTexID = UINT32_MAX;
    uint32_t normalTexID = UINT32_MAX;
    uint32_t emissiveTexID = UINT32_MAX;
    float roughnessFactor = 0.0f;
    float metallicFactor = 0.0f;
};