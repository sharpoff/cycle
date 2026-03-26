#include "core/camera.h"
#include "math/math_helpers.h"

Camera::Camera()
{
    UpdateView();
}

void Camera::SetPosition(vec3 position)
{
    position_ = position;
    UpdateView();
}

void Camera::SetRotation(vec3 rotation)
{
    rotation_ = rotation;
    UpdateView();
}

void Camera::SetPerspectiveInf(float fov, float aspectRatio, float near)
{
    fov_ = fov;
    aspectRatio_ = aspectRatio;
    nearClip_ = near;
    projection_ = math::PerspectiveInf(fov, aspectRatio, near);
}

void Camera::SetPerspective(float fov, float aspectRatio, float near, float far)
{
    fov_ = fov;
    aspectRatio_ = aspectRatio;
    nearClip_ = near;
    farClip_ = far;
    projection_ = math::Perspective(fov, aspectRatio, near, far);
}

void Camera::SetAspectRatio(float aspectRatio)
{
    SetPerspective(fov_, aspectRatio, nearClip_, farClip_);
}

void Camera::UpdateView()
{
    view_ = glm::inverse(glm::translate(position_) * GetRotationMatrix());
}

mat4 Camera::GetRotationMatrix()
{
    quat pitch = glm::angleAxis(rotation_.x, vec3(1.0f, 0.0f, 0.0f));
    quat yaw = glm::angleAxis(rotation_.y, vec3(0.0f, -1.0f, 0.0f));
    quat roll = glm::angleAxis(rotation_.z, vec3(0.0f, 0.0f, 1.0f));

    return glm::toMat4(yaw) * glm::toMat4(roll) * glm::toMat4(pitch);
}