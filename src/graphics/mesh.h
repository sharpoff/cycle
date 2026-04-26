#pragma once

#include "graphics/material.h"
#include "graphics/types.h"
#include "graphics/vulkan_types.h"

struct MeshPrimitive
{
    Vector<Vertex> vertices;
    Vector<uint32_t> indices;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    Material *material = nullptr;
    mat4 worldMatrix = mat4(1.0f);
    uint32_t gltfMaterialIndex;
};

struct Mesh
{
    Vector<MeshPrimitive> primitives;
};