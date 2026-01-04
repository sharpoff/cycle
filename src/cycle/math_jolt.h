#pragma once

#include <Jolt/Jolt.h>
#include "Jolt/Math/Vec3.h"
#include "Jolt/Math/Vec4.h"
#include "Jolt/Math/Mat44.h"

#include "cycle/math.h"

namespace math
{
    inline JPH::Vec4 toJolt(const vec4 &v)
    {
        return JPH::Vec4(v.x, v.y, v.z, v.w);
    }

    inline JPH::Vec3 toJolt(const vec3 &v)
    {
        return JPH::Vec3(v.x, v.y, v.z);
    }

    inline JPH::Mat44 toJolt(const mat4 &m)
    {
        return JPH::Mat44(
            JPH::Vec4Arg(m[0][0], m[0][1], m[0][2], m[0][3]),
            JPH::Vec4Arg(m[1][0], m[1][1], m[1][2], m[1][3]),
            JPH::Vec4Arg(m[2][0], m[2][1], m[2][2], m[2][3]),
            JPH::Vec4Arg(m[3][0], m[3][1], m[3][2], m[3][3])
        );
    }

    inline JPH::Quat toJolt(const quat &q)
    {
        return JPH::Quat(q.x, q.y, q.z, q.w);
    }

    inline vec4 toMath(const JPH::Vec4 &v)
    {
        return vec4(v.GetX(), v.GetY(), v.GetZ(), v.GetW());
    }

    inline vec3 toMath(const JPH::Vec3 &v)
    {
        return vec3(v.GetX(), v.GetY(), v.GetZ());
    }

    inline mat4 toMath(const JPH::Mat44 &m)
    {
        mat4 result = mat4(1.0f);
        m.StoreFloat4x4((JPH::Float4 *)&result);
        return result;
    }

    inline quat toMath(const JPH::Quat &q)
    {
        return quat(q.GetW(), q.GetX(), q.GetY(), q.GetZ());
    }
};