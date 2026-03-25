#pragma once

#include "core/object.h"

#include <Jolt/Jolt.h>

#include "Jolt/Physics/Body/BodyID.h"

class Character : public Object
{
public:
    virtual ~Character();

private:
    JPH::BodyID bodyID;
};