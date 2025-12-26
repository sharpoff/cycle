#version 450
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec3  position;
layout(location = 1) in float uv_x;
layout(location = 2) in vec3  normal;
layout(location = 3) in float uv_y;
layout(location = 4) in vec3  color;

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

// struct Vertex
// {
//     vec3  position;
//     float uv_x;
//     vec3  normal;
//     float uv_y;
//     vec3  color;

//     float _pad0;
// };

// layout (buffer_reference) readonly buffer VertexBuffer {
//     Vertex vertices[];
// };

// layout (buffer_reference) readonly buffer IndexBuffer {
//     uint indices[];
// };

// layout(push_constant) uniform MeshDrawInfo
// {
//     // mat4 worldMatrix;
//     VertexBuffer vertexBuffer;
//     // IndexBuffer indexBuffer;
// } meshDrawInfo;

layout (binding = 0) uniform SceneInfoBuffer
{
    mat4 viewProjection;
} sceneInfo;

void main()
{
    outColor = color;
    outUV = vec2(uv_x, uv_y);
    gl_Position = sceneInfo.viewProjection * vec4(position, 1.0);

    // Vertex vertex = meshDrawInfo.vertexBuffer.vertices[gl_VertexIndex];

    // outColor = vertex.color;
    // outUV = vec2(vertex.uv_x, vertex.uv_y);
    // gl_Position = sceneInfo.viewProjection * vec4(vertex.position, 1.0);
}