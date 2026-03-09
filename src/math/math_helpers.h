#pragma once

#include "math/math_types.h"

namespace math
{
    // https://www.vincentparizet.com/blog/posts/vulkan_perspective_matrix/
    inline mat4 perspective(float fov, float aspectRatio, float near, float far)
    {
        float f = 1.0f / tan(fov * 0.5f);

        // clang-format off
        return mat4(
            f / aspectRatio, 0, 0, 0,
            0, -f, 0, 0,
            0, 0, near / (far - near), -1,
            0, 0, far * near / (far - near), 0
        );
        // clang-format on
    }

    inline mat4 perspectiveInf(float fov, float aspectRatio, float near)
    {
        float f = 1.0f / tanf(fov * 0.5f);

        // clang-format off
        return mat4(
            f / aspectRatio, 0.0f, 0.0f, 0.0f,
            0.0f, -f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, -1.0f,
            0.0f, 0.0f, near, 0.0f
        );
        // clang-format on
    }

    inline vec3 getPosition(const mat4 &m)
    {
        vec3 scale;
        quat orientation;
        vec3 translation;
        vec3 skew;
        vec4 perspective;

        glm::decompose(m, scale, orientation, translation, skew, perspective);

        return translation;
    }

    inline quat getRotation(const mat4 &m)
    {
        vec3 scale;
        quat orientation;
        vec3 translation;
        vec3 skew;
        vec4 perspective;

        glm::decompose(m, scale, orientation, translation, skew, perspective);

        return orientation;
    }

    inline vec3 getScale(const mat4 &m)
    {
        vec3 scale;
        quat orientation;
        vec3 translation;
        vec3 skew;
        vec4 perspective;

        glm::decompose(m, scale, orientation, translation, skew, perspective);

        return scale;
    }

    inline vec3 mouseToDirection(const vec2 &mouseCoords, const vec2 &screenDim, const mat4 &cameraView, const mat4 &cameraProjection)
    {
        double ndc_x = (2.0 * mouseCoords.x / screenDim.x) - 1.0;
        double ndc_y = (2.0 * mouseCoords.y / screenDim.y) - 1.0;

        vec4 clipSpace = vec4(ndc_x, ndc_y, 0.0, 1.0);
        vec4 worldSpace = glm::inverse(cameraProjection * cameraView) * clipSpace;

        vec3 direction = vec3(glm::normalize(worldSpace));
        worldSpace /= worldSpace.w; // perspective division

        return direction;
    }

} // namespace math