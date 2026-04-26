#include "game/entities/static_body.h"

#include "physics/physics.h"

StaticBody::~StaticBody()
{
    gPhysics->UnregisterEntity(this);
}

void StaticBody::CreateBodyFromShape(const PhysicsShape &shape)
{
    if (isShapeCreated)
        return;

    bodyID = gPhysics->CreateStaticBody(shape, GetPosition(), GetRotation());
    isShapeCreated = true;
}

void StaticBody::CreateBodyFromModel()
{
    if (isShapeCreated)
        return;

    Vector<vec3> points = GetModelVertexPositions(GetModel(), GetPosition(), vec3(GetScale()));
    bodyID = gPhysics->CreateStaticBody(ConvexHullShape{.points = points});
    isShapeCreated = true;
}