#pragma once

#include "cycle/math.h"

struct SceneInfo
{
    mat4 view;
    mat4 projection;
    vec3 cameraPos;
    unsigned int lightsCount;
    unsigned int skyboxTexID;
    unsigned int shadowmapTexID;

    float _pad1;
    float _pad2;
};