#ifndef TYPES_GLSL
#define TYPES_GLSL

struct Vertex
{
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
    vec4 tangent;
};

struct Material
{
    uint baseColorTexID;
    uint metallicRoughnessTexID;
    uint normalTexID;
    uint emissiveTexID;
    float roughnessFactor;
    float metallicFactor;
};

struct Light
{
    vec3 position;
    float intensity;
    vec3 direction;
    uint type;
    vec3 color;
};

struct Cascade
{
    mat4 viewProjection;
    float depth;
};

#endif