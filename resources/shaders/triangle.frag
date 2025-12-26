#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

layout (binding = 1) uniform sampler2D texture2Ds[];
#define getTexture2D(id, uv) texture(texture2Ds[nonuniformEXT(id)], uv)

void main()
{
    fragColor = vec4(getTexture2D(0, inUV).rgb, 1.0);
}