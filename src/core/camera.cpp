#include "core/camera.h"
#include "math/math_helpers.h"

Camera::Camera()
{
    UpdateView();
}

void Camera::SetPosition(vec3 position)
{
    m_position = position;
    UpdateView();
}

void Camera::SetRotation(vec3 rotation)
{
    m_rotation = rotation;
    UpdateView();
}

void Camera::SetPerspectiveInf(float fov, float aspectRatio, float near)
{
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_nearClip = near;
    m_projection = math::PerspectiveInf(fov, aspectRatio, near);
}

void Camera::SetPerspective(float fov, float aspectRatio, float near, float far)
{
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_nearClip = near;
    m_farClip = far;
    m_projection = math::Perspective(fov, aspectRatio, near, far);
}

void Camera::SetAspectRatio(float aspectRatio)
{
    SetPerspective(m_fov, aspectRatio, m_nearClip, m_farClip);
}

void Camera::UpdateView()
{
    m_view = glm::inverse(glm::translate(m_position) * GetRotationMatrix());
}

mat4 Camera::GetRotationMatrix()
{
    quat pitch = glm::angleAxis(m_rotation.x, vec3(1.0f, 0.0f, 0.0f));
    quat yaw = glm::angleAxis(m_rotation.y, vec3(0.0f, -1.0f, 0.0f));
    quat roll = glm::angleAxis(m_rotation.z, vec3(0.0f, 0.0f, 1.0f));

    return glm::toMat4(yaw) * glm::toMat4(roll) * glm::toMat4(pitch);
}