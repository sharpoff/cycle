#pragma once

#include "cycle/math.h"

#include "cycle/id_types.h"
#include "cycle/types.h"

class Entity
{
public:
    void setPosition(vec3 position) { this->position = position; }
    void setRotation(quat rotation) { this->rotation = rotation; }
    void setScale(vec3 scale) { this->scale = scale; }

    mat4    getWorldMatrix() { return glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale); }
    ModelID getModelID() { return modelID; }

    vec3 getPosition() { return position; }
    quat getRotation() { return rotation; }
    vec3 getScale() { return scale; }

private:
    String name = "Entity";

    vec3 position = vec3(0.0f);
    quat rotation = quat(1.0f, 0.0f, 0.0f, 0.0f);
    vec3 scale = vec3(1.0f);

    ModelID modelID = ModelID::Invalid;
};