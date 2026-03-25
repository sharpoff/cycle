#pragma once

#include "core/containers.h"
#include "graphics/model.h"
#include "math/math_types.h"

using ModelPtr = std::shared_ptr<Model>;

class Object
{
public:
    enum DrawFlags : uint8_t
    {
        kVisible = 1 << 0,
        kCastShadows = 1 << 1,
    };

public:
    virtual ~Object() = 0;

    virtual void Update(float deltaTime);

    void Translate(vec3 translation) { position_ += translation; }
    void Rotate(quat rotation) { rotation_ *= rotation; }
    void Scale(float scale) { scale_ *= scale; }

    vec3 &GetPosition() { return position_; }
    quat &GetRotation() { return rotation_; }
    float &GetScale() { return scale_; }

    String GetName() { return name_; }
    ModelPtr GetModel() { return model_; }
    uint8_t &GetDrawFlags() { return drawFlags_; }
    mat4 GetWorldMatrix() { return glm::translate(position_) * mat4(rotation_) * glm::scale(vec3(scale_)); }

    void SetPosition(vec3 position) { position_ = position; }
    void SetRotation(quat rotation) { rotation_ = rotation; }
    void SetScale(float scale) { scale_ = scale; }

    void SetName(String name) { name_ = name; }
    void SetModel(ModelPtr model) { model_ = model; }
    void SetDrawFlags(uint8_t newFlags) { drawFlags_ = newFlags; };

protected:
    vec3 position_ = vec3();
    quat rotation_ = glm::identity<quat>();
    float scale_ = 1.0f;

    String name_ = "";

    uint8_t drawFlags_ = kVisible;
    ModelPtr model_ = nullptr;
};