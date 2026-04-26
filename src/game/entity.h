#pragma once

#include "core/containers.h"
#include "graphics/model.h"
#include "math/math_types.h"

class Entity
{
public:
    enum DrawFlags : uint8_t
    {
        kVisible = 1 << 0,
        kCastShadows = 1 << 1,
    };

public:
    virtual ~Entity() = default;

    virtual void Update(float deltaTime) {};

    void Translate(vec3 translation) { m_position += translation; }
    void Rotate(quat rotation) { m_rotation *= rotation; }
    void Scale(float scale) { m_scale *= scale; }

    vec3 &GetPosition() { return m_position; }
    quat &GetRotation() { return m_rotation; }
    float &GetScale() { return m_scale; }

    const String &GetName() { return m_name; }
    Model *GetModel() { return m_model; }
    uint8_t &GetDrawFlags() { return m_drawFlags; }
    mat4 GetWorldMatrix() { return glm::translate(m_position) * mat4(m_rotation) * glm::scale(vec3(m_scale)); }

    void SetPosition(vec3 position) { m_position = position; }
    void SetRotation(quat rotation) { m_rotation = rotation; }
    void SetScale(float scale) { m_scale = scale; }

    void SetName(String name) { m_name = name; }
    void SetModel(Model *model) { m_model = model; }
    void SetDrawFlags(uint8_t newFlags) { m_drawFlags = newFlags; };

protected:
    vec3 m_position = vec3();
    quat m_rotation = glm::identity<quat>();
    float m_scale = 1.0f;

    String m_name = "";

    uint8_t m_drawFlags = kVisible;
    Model *m_model = nullptr;
};