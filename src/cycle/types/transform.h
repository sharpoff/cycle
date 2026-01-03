#pragma once

#include "cycle/math.h"

class Transform
{
public:
    Transform();
    Transform(vec3 position, quat rotation = glm::identity<quat>(), vec3 scale = vec3(1.0f));
    Transform(mat4 matrix);

    void setMatrix(const mat4 &matrix);
    void setPosition(vec3 position);
    void setRotation(quat rotation);
    void setScale(vec3 scale);

    mat4 getMatrix() const { return worldMatrix; }
    vec3 getPosition() const { return position; }
    quat getRotation() const { return rotation; }
    vec3 getScale() const { return scale; }

private:
    void updateWorldMatrix();

    vec3 position = vec3(0.0f);
    quat rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
    vec3 scale = vec3(1.0f);

    mat4 worldMatrix = mat4(1.0f);
};