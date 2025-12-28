#ifndef TEXTURES_GLSL
#define TEXTURES_GLSL

#extension GL_EXT_nonuniform_qualifier : enable

#define SAMPLER_LINEAR_ID 0
#define SAMPLER_NEAREST_ID 1

layout (set = 0, binding = 1) uniform texture2D texture2Ds[];
layout (set = 0, binding = 2) uniform sampler samplers[];

vec4 sampleTexture2DLinear(uint id, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(texture2Ds[id], samplers[SAMPLER_LINEAR_ID])), uv);
}

vec4 sampleTexture2DNearest(uint id, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(texture2Ds[id], samplers[SAMPLER_NEAREST_ID])), uv);
}

#endif