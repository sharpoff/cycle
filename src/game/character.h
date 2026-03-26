#pragma once

#include "game/entity.h"

#include <Jolt/Jolt.h>

#include "Jolt/Physics/Body/BodyID.h"

class Character : public Entity
{
public:
    virtual ~Character() = default;

private:
    JPH::BodyID bodyID;
};