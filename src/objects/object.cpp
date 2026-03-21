#include "object.h"

Object::Object(Object *parent)
    : parent_(parent), position_(vec3()), rotation_(glm::identity<quat>()), scale_(1.0f)
{
}