#pragma once

#include "cycle/components/light_component.h"
#include "cycle/components/name_component.h"
#include "cycle/components/render_component.h"
#include "cycle/components/transform_component.h"
#include "cycle/containers.h"
#include "cycle/managers/component_manager.h"
#include "cycle/types/id.h"

class EntityManager
{
public:
    static void init();
    const EntityID createEntity();

    ComponentManager<TransformComponent> transformComponents;
    ComponentManager<LightComponent> lightComponents;
    ComponentManager<RenderComponent> renderComponents;
    ComponentManager<NameComponent> nameComponents;

    Vector<EntityID> entities;

private:
    EntityManager() {}
    EntityManager(EntityManager &) = delete;
    void operator=(EntityManager const &) = delete;
};