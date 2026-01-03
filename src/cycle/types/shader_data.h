#pragma once

#include "cycle/math.h"

struct SceneInfo
{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
    unsigned int lightsCount;
    unsigned int skyboxTexID;

    float _pad0;
    float _pad1;
    float _pad2;
};

struct MeshDrawInfo
{
    mat4 worldMatrix = mat4(1.0f);
    uint64_t vertexBufferAddress;
    unsigned int materialId;
};