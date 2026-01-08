#version 450

#extension GL_GOOGLE_include_directive : enable

#include "textures.glsl"
#include "materials.glsl"
#include "lights.glsl"
#include "mesh_draw_info.glsl"
#include "scene_info.glsl"
#include "pbr.glsl"

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inWorldPos;

layout(location = 0) out vec4 fragColor;

void main()
{
    Material material = materials[meshDrawInfo.materialID];

    vec3 baseColor = vec3(0.0);
    if (material.baseColorTexID != INVALID_ID) {
        vec4 baseColorTex = sampleTexture2DLinear(material.baseColorTexID, inUV);
        if (baseColorTex.a < 0.5)
            discard;

        baseColor = baseColorTex.rgb;
    }

    float metallic = 0.2;
    float perceptualRoughness = 0.8;
    float reflectance = 0.5;

    if (material.metallicRoughnessTexID != INVALID_ID) {
        vec3 metalRoughnessTex = sampleTexture2DLinear(material.metallicRoughnessTexID, inUV).rgb;
        metallic = metalRoughnessTex.b;
        perceptualRoughness = metalRoughnessTex.g;
    }

    vec3 emissive = vec3(0.0);
    if (material.emissiveTexID != INVALID_ID) {
        emissive = sampleTexture2DLinear(material.emissiveTexID, inUV).rgb;
    }

    vec3 normal = normalize(inNormal);
    vec3 view = normalize(sceneInfo.cameraPos - inWorldPos);

    vec3 diffuseColor = (1.0 - metallic) * baseColor;
    vec3 f0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;

    vec3 lightColor = vec3(0.0);
    for (uint i = 0; i < sceneInfo.lightsCount; i++) {
        Light light = lights[i];

        if (light.type == LIGHT_TYPE_DIRECTIONAL) {
            vec3 l = normalize(-light.direction);
            lightColor += pbrBRDF(view, l, normal, f0, perceptualRoughness, baseColor) * light.color;
        } else if (light.type == LIGHT_TYPE_POINT) {
            vec3 l = normalize(light.position - inWorldPos);
            lightColor += pbrBRDF(view, l, normal, f0, perceptualRoughness, diffuseColor) * light.color;
        }
    }

    vec3 color = lightColor;
    color += emissive;

    fragColor = vec4(color, 1.0);
}