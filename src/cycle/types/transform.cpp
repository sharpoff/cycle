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

    worldMatrix = glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
}

Transform::Transform(mat4 matrix)
{
    setMatrix(matrix);
}

void Transform::setMatrix(const mat4 &matrix)
{
    vec3 skew;
    vec4 perspective;
    if (!glm::decompose(matrix, scale, rotation, position, skew, perspective))
        LOGE("%s", "Failed to decompose model matrix!");

    this->worldMatrix = matrix;
}