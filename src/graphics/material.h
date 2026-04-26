#pragma once

#include "core/constants.h"

struct Material
{
public:
    uint32_t baseColorTextureIndex = InvalidIndex;
    uint32_t metallicRoughnessTextureIndex = InvalidIndex;
    uint32_t normalTextureIndex = InvalidIndex;
    uint32_t emissiveTextureIndex = InvalidIndex;
    float roughnessFactor = 0.0f;
    float metallicFactor = 0.0f;

    void SetID(uint32_t id) { this->id = id; }
    uint32_t GetID() { return id; }

private:
    uint32_t id = UINT32_MAX;
};