#pragma once

#include "math/math_types.h"

class Transform
{
    void SetPosition(const vec3 &position)
    {
        position_ = position;
        CalculateMatrix();
    }

    void SetRotation(const quat &rotation)
    {
        rotation_ = rotation;
        CalculateMatrix();
    }

    void SetScale(const vec3 &scale)
    {
        scale_ = scale;
        CalculateMatrix();
    }

    void Translate(vec3 translation)
    {
        position_ += translation;
        CalculateMatrix();
    }

    void Rotate(quat rotation)
    {
        rotation_ *= rotation;
        CalculateMatrix();
    }

    void Scale(float scale)
    {
        scale_ *= scale;
        CalculateMatrix();
    }

    vec3 &GetPosition() { return position_; }
    quat &GetRotation() { return rotation_; }
    vec3 &GetScale() { return scale_; }

    const mat4 &GetMatrix()
    {
        return matrix_;
    }

private:
    void CalculateMatrix()
    {
        matrix_ = glm::translate(position_) * glm::mat4(rotation_) * glm::scale(scale_);
    }

    vec3 position_ = vec3(0.0f);
    quat rotation_ = glm::identity<quat>();
    vec3 scale_ = vec3(1.0f);

    mat4 matrix_ = mat4(1.0f);
};