#pragma once

#include "graphics/mesh.h"
#include "math/bounds.h"

struct Model
{
    Bounds bounds;
    Vector<Mesh> meshes;
};

Vector<vec3> GetModelVertexPositions(Model *model, vec3 position = vec3(0.0f), vec3 scale = vec3(1.0f));