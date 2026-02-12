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
layout(location = 3) in vec4 inTangent;
layout(location = 4) in mat3 inTBN;

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

    float metallic = 0.5;
    float perceptualRoughness = 0.7;

    if (material.metallicRoughnessTexID != INVALID_ID) {
        vec3 metalRoughnessTex = sampleTexture2DLinear(material.metallicRoughnessTexID, inUV).rgb;
        metallic = material.metallicFactor * metalRoughnessTex.b;
        perceptualRoughness = material.roughnessFactor * metalRoughnessTex.g;
    }

    vec3 normal = normalize(inNormal);
    if (inTangent != vec4(0.0) && material.normalTexID != INVALID_ID) {
        normal = sampleTexture2DLinear(material.normalTexID, inUV).rgb;
        normal = normalize(inTBN * normalize(normal * 2.0 - 1.0));
    }

    vec3 diffuseColor = (1.0 - metallic) * baseColor;

    float reflectance = 0.5;
    vec3 f0 = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;
    vec3 v = normalize(sceneInfo.cameraPos - inWorldPos);

    vec3 lightColor = vec3(0.0);
    for (uint i = 0; i < sceneInfo.lightsCount; i++) {
        Light light = lights[i];

        vec3 l = normalize(-light.direction);
        if (light.type != LIGHT_TYPE_DIRECTIONAL) {
            l = normalize(light.position - inWorldPos);
        }

        lightColor += pbrBRDF(v, l, normal, f0, perceptualRoughness, diffuseColor) * light.color;
    }

    vec3 ambient = baseColor * 0.02;
    vec3 color = lightColor + ambient;

    if (material.emissiveTexID != INVALID_ID) {
        vec3 emissive = sampleTexture2DLinear(material.emissiveTexID, inUV).rgb;
        color += emissive;
    }

#if 0
    color = normal;
#endif

    fragColor = vec4(color, 1.0);
}