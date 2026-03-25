#pragma once

#include "graphics/material.h"
#include "graphics/vulkan_types.h"
#include "graphics/vertex.h"

using BufferPtr = std::shared_ptr<Buffer>;
using MaterialPtr = std::shared_ptr<Material>;

struct MeshPrimitive
{
    Vector<Vertex> vertices;
    Vector<uint32_t> indices;
    BufferPtr vertexBuffer = nullptr;
    BufferPtr indexBuffer = nullptr;
    MaterialPtr material = nullptr;
    mat4 worldMatrix = mat4(1.0f);
    uint32_t gltfMaterialIndex;
};

struct Mesh
{
    Vector<MeshPrimitive> primitives;
};