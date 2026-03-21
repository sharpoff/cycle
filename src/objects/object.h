#pragma once

#include "core/containers.h"
#include "graphics/id.h"
#include "math/math_types.h"

class Object
{
public:
    enum DrawFlags
    {
        kVisible = 0,
    };

public:
    Object(Object *parent = nullptr);
    virtual ~Object() = default;

    bool hasChildren() { return !children.empty(); }

    vec3 &getPosition() { return position_; }
    quat &getRotation() { return rotation_; }
    float &getScale() { return scale_; }
    Object *getParent() { return parent_; }
    DrawFlags &getDrawFlags() { return drawFlags_; }
    Vector<Object *> &getChildren() { return children; }
    const ModelID &getModelID() { return modelID_; }
    mat4 getWorldMatrix() { return glm::translate(position_) * mat4(rotation_) * glm::scale(vec3(scale_)); }

    void setPosition(vec3 position) { position_ = position; }
    void setRotation(quat rotation) { rotation_ = rotation; }
    void setScale(float scale) { scale_ = scale; }
    void setModelID(const ModelID id) { modelID_ = id; }
    void setDrawFlags(DrawFlags newFlags) { drawFlags_ = newFlags; };

protected:
    vec3 position_;
    quat rotation_;
    float scale_;

    DrawFlags drawFlags_ = kVisible;
    ModelID modelID_ = ModelID::Invalid;

    // for scene graph
    Object *parent_ = nullptr;
    Vector<Object *> children;
};