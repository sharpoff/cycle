#version 450

#extension GL_GOOGLE_include_directive : enable

#include "mesh_draw_info.glsl"
#include "scene_info.glsl"

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outWorldPos;
layout(location = 3) out vec4 outTangent;
layout(location = 4) out mat3 outTBN;

void main()
{
    Vertex vertex = meshDrawInfo.vertexBuffer.vertices[gl_VertexIndex];
    vec4 world = meshDrawInfo.worldMatrix * vec4(vertex.position, 1.0);

    gl_Position = sceneInfo.projection * sceneInfo.view * world;

    outUV = vec2(vertex.uv_x, vertex.uv_y);
    outTangent = vertex.tangent;
    outNormal = mat3(transpose(inverse(meshDrawInfo.worldMatrix))) * vertex.normal;
    outWorldPos = vec3(world);

    vec3 N = normalize(outNormal);
    vec3 T = normalize(vec3(meshDrawInfo.worldMatrix * vertex.tangent));
    vec3 B = cross(N, T) * vertex.tangent.w;
    outTBN = mat3(T, B, N);
}