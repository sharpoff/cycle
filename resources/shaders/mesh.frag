#version 450

#extension GL_GOOGLE_include_directive : enable

#include "textures.glsl"
#include "materials.glsl"
#include "mesh_draw_info.glsl"
#include "pbr.glsl"

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;

void main()
{
    Material material = materials[meshDrawInfo.materialID];
    if (meshDrawInfo.materialID == 0) { // default material
        fragColor = vec4(sampleTexture2DLinear(material.baseColorTexID, inUV));
        return;
    }

    vec3 color = vec3(0.0);
    vec3 diffuseColor = vec3(0.0);

    if (material.baseColorTexID > 0)
        diffuseColor = sampleTexture2DLinear(material.baseColorTexID, inUV).rgb;

    color = diffuseColor;

    fragColor = vec4(color, 1.0);
}