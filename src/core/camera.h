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

    mat4 GetProjection() { return projection; }
    mat4 GetView() { return view; }
    vec3 GetDirection() { return vec3(view[0][2], view[1][2], view[2][2]); }

    mat4 GetRotation();
    vec3 GetPosition() { return position; }

    float GetFov() { return fov; }
    float GetAspectRatio() { return aspectRatio; }
    float GetNearClip() { return nearClip; }
    float GetFarClip() { return farClip; }

private:
    void UpdateView();

    mat4 projection = mat4(1.0f);
    mat4 view = mat4(1.0f);

    float fov = 60.0f;
    float aspectRatio = 0.0f;
    float nearClip = 0.01f;
    float farClip = 1000.0f;

    vec3 position = vec3(0.0f);
    vec3 rotation = vec3(0.0f);
};