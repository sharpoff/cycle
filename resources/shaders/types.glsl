#ifndef TYPES_GLSL
#define TYPES_GLSL

// Rendering Flags
const uint RENDERING_NONE = 0;
const uint RENDERING_FILL = 1;
const uint RENDERING_WIREFRAME = 2;
const uint RENDERING_LIGHTING = 1 << 1;
const uint RENDERING_SHADOWS = 1 << 2;
const uint RENDERING_MATERIAL = 1 << 3;
const uint RENDERING_ALL = RENDERING_FILL | RENDERING_LIGHTING | RENDERING_SHADOWS | RENDERING_MATERIAL;

// Vertex
struct Vertex
{
    vec3  position;
    float uv_x;
    vec3  normal;
    float uv_y;
    vec3  color;

    float _pad0;
};

#endif