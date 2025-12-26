#pragma once

#include "cycle/types/model.h"

class ResourceManager;

namespace gltf
{
    bool loadModel(ResourceManager *resourceManager, Model &model, const String &filename);
}