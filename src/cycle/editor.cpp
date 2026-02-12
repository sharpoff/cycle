#include "cycle/editor.h"

#include "cycle/input.h"
#include "cycle/physics.h"
#include "cycle/renderer.h"
#include "cycle/managers/entity_manager.h"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

Editor *g_editor;

void Editor::init()
{
    static Editor instance;
    g_editor = &instance;
}

Editor *Editor::get()
{
    return g_editor;
}

void Editor::processInput()
{
    if (isKeyboardProcessed) {
        if (Input::get()->isKeyDown(KeyboardKey::T)) {
            gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
        }
        if (Input::get()->isKeyDown(KeyboardKey::E)) {
            gizmoOperation = ImGuizmo::OPERATION::SCALE;
        }
        if (Input::get()->isKeyDown(KeyboardKey::R)) {
            gizmoOperation = ImGuizmo::OPERATION::ROTATE;
        }
        if (Input::get()->isKeyDown(KeyboardKey::Y)) {
            gizmoOperation = ImGuizmo::OPERATION::UNIVERSAL;
        }
    }

    if (isMouseProcessed) {
        if (Input::get()->isMouseButtonDown(MouseButton::RIGHT)) { // raycast to find an entity at mouse position
            vec3 direction = math::mouseToDirection(Input::get()->getMousePosition(), Renderer::get()->getScreenSize(), camera->getView(), camera->getProjection());
            direction *= 1000.0f;
            const EntityID entityID = Physics::get()->castRay(camera->getPosition(), direction);
            selectedEntityID = entityID;
        }
    }
}

void Editor::draw()
{
    assert(camera);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    updateGizmo();

    {
        ImGui::Begin("Entities");

        for (EntityID entity : EntityManager::get()->transforms.getEntities()) {
            TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(entity);
            NameComponent *nameComponent = EntityManager::get()->names.getComponent(entity);
            if (!transformComponent || !nameComponent)
                continue;

            if (ImGui::Selectable(nameComponent->name.c_str(), entity == selectedEntityID)) {
                if (entity != selectedEntityID)
                    selectedEntityID = entity;
                else
                    selectedEntityID = EntityID::Invalid;
            }
        }

        ImGui::End();
    }

    ImGui::ShowDemoWindow();

    ImGui::Render();
}

void Editor::updateGizmo()
{
    ImGuizmo::BeginFrame();
    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    if (selectedEntityID != EntityID::Invalid) {
        TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(selectedEntityID);
        if (!transformComponent)
            return;

        mat4 view = camera->getView();
        // NOTE: ImGuizmo does not support reverse depth perspective so recreate it
        mat4 projection = glm::perspective(camera->getFov(), camera->getAspectRatio(), camera->getNearClip(), 1000.0f);

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmoOperation, gizmoMode, glm::value_ptr(transformComponent->transform), NULL, NULL)) {
            lastFrameManipulated = true;
        }

        if (!ImGuizmo::IsUsingAny() && lastFrameManipulated) {
            if (EntityManager::get()->rigidBodies.has(selectedEntityID)) {
                Physics::get()->setBodyPosition(selectedEntityID, math::getPosition(transformComponent->transform));
            }

            lastFrameManipulated = false;
        }
    }
}