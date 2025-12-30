#pragma once

#include "cycle/math.h"

class Transform
{
public:
    Transform();
    Transform(vec3 position, quat rotation, vec3 scale);
    Transform(mat4 matrix);

    void setMatrix(const mat4 &matrix);

    const mat4 &getMatrix() const { return worldMatrix; }

private:
    vec3 position = vec3(0.0f);
    quat rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
    vec3 scale = vec3(1.0f);

    mat4 worldMatrix = mat4(1.0f);
};