#ifndef TEXTURES_GLSL
#define TEXTURES_GLSL

#extension GL_EXT_nonuniform_qualifier : enable

const int SAMPLER_LINEAR_ID = 0;
const int SAMPLER_NEAREST_ID = 1;

layout (set = 0, binding = 1) uniform texture2D textures2D[];
layout (set = 0, binding = 1) uniform textureCube texturesCube[];
layout (set = 0, binding = 2) uniform sampler samplers[];

vec4 sampleTexture2DLinear(uint id, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(textures2D[id], samplers[SAMPLER_LINEAR_ID])), uv);
}

vec4 sampleTexture2DNearest(uint id, vec2 uv) {
    return texture(nonuniformEXT(sampler2D(textures2D[id], samplers[SAMPLER_NEAREST_ID])), uv);
}

vec4 sampleTextureCubeLinear(uint id, vec3 uv) {
    return texture(nonuniformEXT(samplerCube(texturesCube[id], samplers[SAMPLER_LINEAR_ID])), uv);
}

vec4 sampleTextureCubeNearest(uint id, vec3 uv) {
    return texture(nonuniformEXT(samplerCube(texturesCube[id], samplers[SAMPLER_NEAREST_ID])), uv);
}

#endif