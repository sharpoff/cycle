#pragma once

#include "core/gltf_scene.h"
#include "graphics/model.h"

namespace gltf
{
    // converts all gltf scene meshes to model
    bool ConvertToModel(Model * model, Scene &scene);
} // namespace gltf