#pragma once

#include <Jolt/Jolt.h>
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"

// Layer that objects can be in, determines which other objects it can collide with
namespace PhysicsLayers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
} // namespace Layers

// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override
	{
        switch (inLayer1) {
            case PhysicsLayers::NON_MOVING:
                return inLayer2 == PhysicsLayers::MOVING;
            case PhysicsLayers::MOVING:
                return true;
            default:
                printf("[PHYSICS] Invalid object layer\n");
                return false;
        }
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase.
namespace PhysicsBroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        // Create mapping table from object to broad phase layer
        objectToBroadPhase[PhysicsLayers::NON_MOVING] = PhysicsBroadPhaseLayers::NON_MOVING;
        objectToBroadPhase[PhysicsLayers::MOVING] = PhysicsBroadPhaseLayers::MOVING;
    }

	virtual uint GetNumBroadPhaseLayers() const override
    {
        return PhysicsBroadPhaseLayers::NUM_LAYERS;
    }

	/// Convert an object layer to the corresponding broadphase layer
	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        return objectToBroadPhase[inLayer];
    }

	virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {
            case (static_cast<JPH::BroadPhaseLayer::Type>(PhysicsBroadPhaseLayers::NON_MOVING)):
                return "NON_MOVING";
            case (static_cast<JPH::BroadPhaseLayer::Type>(PhysicsBroadPhaseLayers::MOVING)):
                return "MOVING";
            default:
                printf("[PHYSICS] Invalid broad phase layer name\n");
                return "INVALID";
        }
    }

private:
    JPH::BroadPhaseLayer objectToBroadPhase[PhysicsLayers::NUM_LAYERS];
};

// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1) {
            case PhysicsLayers::NON_MOVING:
                return inLayer2 == PhysicsBroadPhaseLayers::MOVING;
            case PhysicsLayers::MOVING:
                return true;
            default:
                printf("[PHYSICS] Invalid object or broad phase layer\n");
                return false;
        }
    }
};
