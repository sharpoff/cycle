#pragma once

#include "cycle/graphics/graphics_types.h"
#include "cycle/types/id.h"
#include "cycle/math.h"
#include "cycle/containers.h"
#include "cycle/types/vertex.h"

struct Mesh
{
    Vector<Vertex> vertices;
    Vector<uint32_t> indices;
    Buffer vertexBuffer;
    Buffer indexBuffer;

    mat4 worldMatrix = mat4(1.0f);
    MaterialID materialID = MaterialID::Invalid;
};