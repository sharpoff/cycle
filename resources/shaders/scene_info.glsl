#ifndef SCENE_INFO_GLSL
#define SCENE_INFO_GLSL

layout (set = 0, binding = 0) uniform SceneInfoBuffer
{
    mat4 viewProjection;
} sceneInfo;

#endif