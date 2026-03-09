#version 450

#extension GL_GOOGLE_include_directive : enable

#extension GL_EXT_buffer_reference : require

#include "types.glsl"

layout (buffer_reference) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform DepthPushConstants
{
    mat4 worldMatrix;
    VertexBuffer vertexBuffer;
    uint cascadeIndex;
} pcs;

layout(set = 0, binding = 5) uniform CascadesBuffer
{
    Cascade cascades[SHADOWMAP_CASCADES];
};

layout(location = 0) out vec2 outUV;

void main()
{
    Vertex vertex = pcs.vertexBuffer.vertices[gl_VertexIndex];

    outUV = vec2(vertex.uv_x, vertex.uv_y);
    vec4 world = pcs.worldMatrix * vec4(vertex.position, 1.0);
    gl_Position = cascades[pcs.cascadeIndex].viewProjection * world;
}