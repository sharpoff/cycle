#ifndef MESH_DRAW_INFO_GLSL
#define MESH_DRAW_INFO_GLSL

#extension GL_EXT_buffer_reference : require

#include "types.glsl"

layout (buffer_reference) readonly buffer VertexBuffer {
    Vertex vertices[];
};

// layout (buffer_reference) readonly buffer IndexBuffer {
//     uint indices[];
// };

layout(push_constant) uniform MeshDrawInfo
{
    mat4 worldMatrix;
    VertexBuffer vertexBuffer;
    // IndexBuffer indexBuffer;
} meshDrawInfo;

#endif