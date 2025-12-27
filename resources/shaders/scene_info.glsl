#ifndef SCENE_INFO_GLSL
#define SCENE_INFO_GLSL

layout (binding = 0) uniform SceneInfoBuffer
{
    mat4 viewProjection;
} sceneInfo;

#endif