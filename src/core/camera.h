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

    mat4 GetProjection() { return projection_; }
    mat4 GetView() { return view_; }
    vec3 GetDirection() { return vec3(view_[0][2], view_[1][2], view_[2][2]); }

    vec3 GetPosition() { return position_; }
    vec3 GetRotation() { return rotation_; }
    mat4 GetRotationMatrix();

    float GetFov() { return fov_; }
    float GetAspectRatio() { return aspectRatio_; }
    float GetNearClip() { return nearClip_; }
    float GetFarClip() { return farClip_; }

private:
    void UpdateView();

    mat4 projection_ = mat4(1.0f);
    mat4 view_ = mat4(1.0f);

    float fov_ = 60.0f;
    float aspectRatio_ = 0.0f;
    float nearClip_ = 0.01f;
    float farClip_ = 1000.0f;

    vec3 position_ = vec3(0.0f);
    vec3 rotation_ = vec3(0.0f);
};