#pragma once

#include "math/math_types.h"

class Camera
{
public:
    Camera();

    void SetPosition(vec3 position);
    void SetRotation(vec3 rotation);
    void SetPerspectiveInf(float fov, float aspectRatio, float near);
    void SetPerspective(float fov, float aspectRatio, float near, float far);
    void SetAspectRatio(float aspectRatio);

    mat4 GetProjection() { return m_projection; }
    mat4 GetView() { return m_view; }
    vec3 GetDirection() { return vec3(m_view[0][2], m_view[1][2], m_view[2][2]); }

    vec3 GetPosition() { return m_position; }
    vec3 GetRotation() { return m_rotation; }
    mat4 GetRotationMatrix();

    float GetFov() { return m_fov; }
    float GetAspectRatio() { return m_aspectRatio; }
    float GetNearClip() { return m_nearClip; }
    float GetFarClip() { return m_farClip; }

private:
    void UpdateView();

    mat4 m_projection = mat4(1.0f);
    mat4 m_view = mat4(1.0f);

    float m_fov = 60.0f;
    float m_aspectRatio = 0.0f;
    float m_nearClip = 0.01f;
    float m_farClip = 1000.0f;

    vec3 m_position = vec3(0.0f);
    vec3 m_rotation = vec3(0.0f);
};