#version 450

#extension GL_GOOGLE_include_directive : enable

#include "mesh_draw_info.glsl"
#include "scene_info.glsl"

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outWorldPos;

void main()
{
    Vertex vertex = meshDrawInfo.vertexBuffer.vertices[gl_VertexIndex];

    outUV = vec2(vertex.uv_x, vertex.uv_y);
    outNormal = mat3(meshDrawInfo.worldMatrix) * vertex.normal;

    vec4 world = meshDrawInfo.worldMatrix * vec4(vertex.position, 1.0);
    gl_Position = sceneInfo.projection * sceneInfo.view * world;
    outWorldPos = vec3(world);
}