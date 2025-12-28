#ifndef TYPES_GLSL
#define TYPES_GLSL

// Rendering flags
const uint RENDERING_NONE = 0;
const uint RENDERING_FILL = 1;
const uint RENDERING_WIREFRAME = 2;
const uint RENDERING_LIGHTING = 1 << 1;
const uint RENDERING_SHADOWS = 1 << 2;
const uint RENDERING_MATERIAL = 1 << 3;
const uint RENDERING_ALL = RENDERING_FILL | RENDERING_LIGHTING | RENDERING_SHADOWS | RENDERING_MATERIAL;

#define DEFAULT_TEXTURE_ID 0
#define DEFAULT_MATERIAL_ID 0

struct Vertex
{
    vec3  position;
    float uv_x;
    vec3  normal;
    float uv_y;
};

struct Material
{
    uint baseColorTexID;
    uint metallicRoughnessTexID;
    uint normalTexID;
    uint emissiveTexID;
};

#endif