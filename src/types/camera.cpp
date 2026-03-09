#include "types/camera.h"
#include "math/math_helpers.h"

Camera::Camera()
{
    updateView();
}

void Camera::move(vec3 translation)
{
    if (translation.x != 0.0f || translation.y != 0.0f || translation.z != 0.0f) {
        this->position += translation;
        updateView();
    }
}

void Camera::rotate(vec3 rotation)
{
    if (rotation.x != 0.0f || rotation.y != 0.0f || rotation.z != 0.0f) {
        this->rotation += rotation;
        this->rotation.x = glm::clamp(this->rotation.x, glm::radians(-89.9f), glm::radians(89.9f));
        updateView();
    }
}

void Camera::setPosition(vec3 position)
{
    this->position = position;
    updateView();
}

void Camera::setRotation(vec3 rotation)
{
    this->rotation = rotation;
    updateView();
}

void Camera::setPerspectiveInf(float fov, float aspectRatio, float near)
{
    this->fov = fov;
    this->aspectRatio = aspectRatio;
    this->nearClip = near;
    this->projection = math::perspectiveInf(fov, aspectRatio, near);
}

void Camera::setPerspective(float fov, float aspectRatio, float near, float far)
{
    this->fov = fov;
    this->aspectRatio = aspectRatio;
    this->nearClip = near;
    this->farClip = far;
    this->projection = math::perspective(fov, aspectRatio, near, far);
}

void Camera::setAspectRatio(float aspectRatio)
{
    setPerspective(fov, aspectRatio, nearClip, farClip);
}

void Camera::updateView()
{
    view = glm::inverse(glm::translate(position) * getRotation());
}

mat4 Camera::getRotation()
{
    quat pitch = glm::angleAxis(rotation.x, vec3(1.0f, 0.0f, 0.0f));
    quat yaw = glm::angleAxis(rotation.y, vec3(0.0f, -1.0f, 0.0f));
    quat roll = glm::angleAxis(rotation.z, vec3(0.0f, 0.0f, 1.0f));

    return glm::toMat4(yaw) * glm::toMat4(roll) * glm::toMat4(pitch);
}