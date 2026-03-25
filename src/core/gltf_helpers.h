#pragma once

#include "core/gltf_scene.h"
#include "graphics/model.h"

using ModelPtr = std::shared_ptr<Model>;

namespace gltf
{
    // converts all gltf scene meshes to model
    bool ConvertToModel(ModelPtr model, Scene &scene);
} // namespace gltf