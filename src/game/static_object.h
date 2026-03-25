// static object - not movable physics object
#pragma once

#include <Jolt/Jolt.h>

#include "Jolt/Physics/Body/BodyID.h"

#include "core/object.h"

class StaticObject : public Object
{
public:
    StaticObject(const JPH::BodyID &bodyID)
        : bodyID(bodyID) {}

    void SetConvexFromModel(bool value);

    JPH::BodyID &GetBodyId() { return bodyID; }

protected:
    JPH::BodyID bodyID;
};