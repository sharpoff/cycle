#include "cycle/editor.h"

#include "cycle/input.h"
#include "cycle/physics.h"
#include "cycle/renderer.h"
#include "cycle/managers/entity_manager.h"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

extern Input *g_input;
extern EntityManager *g_entityManager;
extern Physics *g_physics;
extern Renderer *g_renderer;

Editor *g_editor;

void Editor::init()
{
    static Editor instance;
    g_editor = &instance;
}

void Editor::processInput()
{
    if (isKeyboardProcessed) {
        if (g_input->isKeyDown(KeyboardKey::T)) {
            gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
        }
        if (g_input->isKeyDown(KeyboardKey::E)) {
            gizmoOperation = ImGuizmo::OPERATION::SCALE;
        }
        if (g_input->isKeyDown(KeyboardKey::R)) {
            gizmoOperation = ImGuizmo::OPERATION::ROTATE;
        }
        if (g_input->isKeyDown(KeyboardKey::Y)) {
            gizmoOperation = ImGuizmo::OPERATION::UNIVERSAL;
        }
    }

    if (isMouseProcessed) {
        if (g_input->isMouseButtonDown(MouseButton::RIGHT)) { // raycast to find an entity at mouse position
            vec3 direction = math::mouseToDirection(g_input->getMousePosition(), g_renderer->getScreenSize(), camera->getView(), camera->getProjection());
            direction *= 1000.0f;
            const EntityID entityID = g_physics->castRay(camera->getPosition(), direction);
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

        for (EntityID entity : g_entityManager->transforms.getEntities()) {
            TransformComponent *transformComponent = g_entityManager->transforms.getComponent(entity);
            NameComponent *nameComponent = g_entityManager->names.getComponent(entity);
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
        TransformComponent *transformComponent = g_entityManager->transforms.getComponent(selectedEntityID);
        if (!transformComponent)
            return;

        mat4 view = camera->getView();
        // NOTE: ImGuizmo does not support reverse depth perspective so recreate it
        mat4 projection = glm::perspective(camera->getFov(), camera->getAspectRatio(), camera->getNearClip(), 1000.0f);

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmoOperation, gizmoMode, glm::value_ptr(transformComponent->transform), NULL, NULL)) {
            lastFrameManipulated = true;
        }

        if (!ImGuizmo::IsUsingAny() && lastFrameManipulated) {
            if (g_entityManager->rigidBodies.has(selectedEntityID)) {
                g_physics->setBodyPosition(selectedEntityID, math::getPosition(transformComponent->transform));
            }

            lastFrameManipulated = false;
        }
    }
}