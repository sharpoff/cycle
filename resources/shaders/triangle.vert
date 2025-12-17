#version 450

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outUV;

struct Vertex
{
    vec3 position;
    float uv_x;
    vec3 color;
    float uv_y;
};

layout (binding = 1) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout (binding = 2) uniform GlobalData
{
    mat4 viewProjection;
};

void main()
{
    Vertex vertex = vertices[gl_VertexIndex];

    outColor = vertex.color;
    outUV = vec2(vertex.uv_x, vertex.uv_y);
    gl_Position = viewProjection * vec4(vertex.position, 1.0);
}