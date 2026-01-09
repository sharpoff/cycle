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

const int SHADOW_MAP_CASCADES = 4;

const int INVALID_ID = 4294967295;

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

// Light types
const uint LIGHT_TYPE_DIRECTIONAL = 0;
const uint LIGHT_TYPE_POINT = 1;
const uint LIGHT_TYPE_SPOT = 2;

struct Light
{
    vec3  position;
    float intensity;
    vec3  direction;
    uint  type;
    vec3  color;
};

#endif