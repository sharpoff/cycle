#include "cycle/types/transform.h"
#include "cycle/logger.h"
#include "cycle/math.h"

Transform::Transform()
{
    position = vec3(0.0f);
    rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
    scale = vec3(1.0f);
    worldMatrix = mat4(1.0f);
}

Transform::Transform(vec3 position, quat rotation, vec3 scale)
{
    this->position = position;
    this->rotation = rotation;
    this->scale = scale;
    updateWorldMatrix();
}

Transform::Transform(mat4 matrix)
{
    setMatrix(matrix);
}

void Transform::setMatrix(const mat4 &matrix)
{
    vec3 skew;
    vec4 perspective;
    vec3 scale;
    quat orientation;
    vec3 translation;

    if (!glm::decompose(matrix, scale, orientation, translation, skew, perspective)) {
        LOGE("%s", "Failed to decompose model matrix!");
        return;
    }

    this->position = translation;
    this->rotation = orientation;
    this->scale = scale;
    updateWorldMatrix();
}

void Transform::setPosition(vec3 position)
{
    this->position = position;
    updateWorldMatrix();
}

void Transform::setRotation(quat rotation)
{
    this->rotation = rotation;
    updateWorldMatrix();
}

void Transform::setScale(vec3 scale)
{
    this->scale = scale;
    updateWorldMatrix();
}

void Transform::updateWorldMatrix()
{
    worldMatrix = glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
}