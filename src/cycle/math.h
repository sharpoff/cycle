#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/integer.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::quat;
using glm::vec2;
using glm::vec3;
using glm::vec4;

const float MATH_PI = glm::pi<float>();

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