#version 450

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : enable

#include "common/textures.glsl"
#include "common/scene_info.glsl"
#include "common/types.glsl"

layout (location = 0) in vec3 inUVW;
layout (location = 0) out vec4 fragColor;

void main()
{
    if (sceneInfo.skyboxTexID != INVALID_ID)
        fragColor = sampleTextureCubeLinear(sceneInfo.skyboxTexID, inUVW);
    else
        fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}