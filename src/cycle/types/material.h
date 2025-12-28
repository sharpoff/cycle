#pragma once

#include "cycle/types/id.h"

struct Material
{
    TextureID baseColorTexID = TextureID::Invalid;
    TextureID metallicRoughnessTexID = TextureID::Invalid;
    TextureID normalTexID = TextureID::Invalid;
    TextureID emissiveTexID = TextureID::Invalid;
};