#ifndef SCENE_INFO_GLSL
#define SCENE_INFO_GLSL

layout (set = 0, binding = 0) uniform SceneInfoBuffer
{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
    uint lightsCount;
    uint skyboxTexID;
} sceneInfo;

#endif