#pragma once

#include "types/id.h"
#include "graphics/vulkan_types.h"
#include "types/vertex.h"

struct MeshPrimitive
{
    Vector<Vertex> vertices;
    Vector<uint32_t> indices;
    Buffer vertexBuffer;
    Buffer indexBuffer;

    mat4 worldMatrix = mat4(1.0f);
    MaterialID materialID = MaterialID::Invalid;
    uint32_t materialIndex;
};

struct Mesh
{
    String name = "";
    Vector<MeshPrimitive> primitives;
};