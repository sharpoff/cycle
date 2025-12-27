#version 450

#extension GL_GOOGLE_include_directive : enable

#include "mesh_draw_info.glsl"
#include "scene_info.glsl"

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

void main()
{
    Vertex vertex = meshDrawInfo.vertexBuffer.vertices[gl_VertexIndex];

    outColor = vertex.normal;
    outUV = vec2(vertex.uv_x, vertex.uv_y);
    gl_Position = sceneInfo.viewProjection * meshDrawInfo.worldMatrix * vec4(vertex.position, 1.0);
}