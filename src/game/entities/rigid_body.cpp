#include "game/entities/rigid_body.h"

#include "physics/physics.h"

RigidBody::~RigidBody()
{
    gPhysics->UnregisterEntity(this);
}

void RigidBody::CreateBodyFromShape(const PhysicsShape &shape)
{
    if (isShapeCreated)
        return;

    bodyID = gPhysics->CreateDynamicBody(shape, GetPosition(), GetRotation());
    isShapeCreated = true;
}

void RigidBody::CreateBodyFromModel()
{
    if (isShapeCreated)
        return;

    Vector<vec3> points = GetModelVertexPositions(GetModel(), GetPosition(), vec3(GetScale()));
    bodyID = gPhysics->CreateDynamicBody(ConvexHullShape{.points = points}, GetPosition(), GetRotation());
    isShapeCreated = true;
}