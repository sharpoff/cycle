#version 450

#extension GL_GOOGLE_include_directive : enable

#include "mesh_draw_info.glsl"
#include "scene_info.glsl"

layout (location = 0) out vec3 outUVW;

void main()
{
    Vertex vertex = meshDrawInfo.vertexBuffer.vertices[gl_VertexIndex];

    outUVW = vertex.position;
    mat4 view = mat4(mat3(sceneInfo.view)); // remove translation

    gl_Position = sceneInfo.projection * view * vec4(vertex.position, 1.0);
}