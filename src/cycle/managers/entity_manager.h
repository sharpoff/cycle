#pragma once

#include "cycle/components/light_component.h"
#include "cycle/components/name_component.h"
#include "cycle/components/model_component.h"
#include "cycle/components/rigid_body_component.h"
#include "cycle/components/transform_component.h"
#include "cycle/components/component_holder.h"
#include "cycle/containers.h"
#include "cycle/types/id.h"

class EntityManager
{
public:
    static void    init();
    static EntityManager *get();

    const EntityID createEntity();
    void destroyEntity(const EntityID id);

    const EntityID getEntityIDByName(String name);

    ComponentHolder<TransformComponent> transforms;
    ComponentHolder<LightComponent>     lights;
    ComponentHolder<ModelComponent>     models;
    ComponentHolder<NameComponent>      names;
    ComponentHolder<RigidBodyComponent> rigidBodies;

    Vector<EntityID> entities;

private:
    EntityManager() {}
    EntityManager(EntityManager &) = delete;
    void operator=(EntityManager const &) = delete;

    Queue<size_t> freeList;
};