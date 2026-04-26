#pragma once

#include "math/math_types.h"

class Transform
{
    void SetPosition(const vec3 &position)
    {
        m_position = position;
        CalculateMatrix();
    }

    void SetRotation(const quat &rotation)
    {
        m_rotation = rotation;
        CalculateMatrix();
    }

    void SetScale(const vec3 &scale)
    {
        m_scale = scale;
        CalculateMatrix();
    }

    void Translate(vec3 translation)
    {
        m_position += translation;
        CalculateMatrix();
    }

    void Rotate(quat rotation)
    {
        m_rotation *= rotation;
        CalculateMatrix();
    }

    void Scale(float scale)
    {
        m_scale *= scale;
        CalculateMatrix();
    }

    vec3 &GetPosition() { return m_position; }
    quat &GetRotation() { return m_rotation; }
    vec3 &GetScale() { return m_scale; }

    const mat4 &GetMatrix()
    {
        return m_matrix;
    }

private:
    void CalculateMatrix()
    {
        m_matrix = glm::translate(m_position) * glm::mat4(m_rotation) * glm::scale(m_scale);
    }

    vec3 m_position = vec3(0.0f);
    quat m_rotation = glm::identity<quat>();
    vec3 m_scale = vec3(1.0f);

    mat4 m_matrix = mat4(1.0f);
};