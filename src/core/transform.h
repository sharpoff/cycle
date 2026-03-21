#pragma once

#include "math/math_types.h"

class Transform
{
    void setPosition(const vec3 &newPosition)
    {
        position = newPosition;
        calculateMatrix();
    }

    void setRotation(const quat &newRotation)
    {
        rotation = newRotation;
        calculateMatrix();
    }

    void setScale(const vec3 &newScale)
    {
        scale = newScale;
        calculateMatrix();
    }

    vec3 getPosition() { return position; }
    quat getRotation() { return rotation; }
    vec3 getScale() { return scale; }

    const mat4 &getMatrix()
    {
        return matrix;
    }

private:
    void calculateMatrix()
    {
        matrix = glm::translate(position) * glm::mat4(rotation) * glm::scale(scale);
    }

    vec3 position = vec3(0.0f);
    quat rotation = glm::identity<quat>();
    vec3 scale = vec3(1.0f);

    mat4 matrix = mat4(1.0f);
};