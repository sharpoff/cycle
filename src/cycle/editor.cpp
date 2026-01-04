#include "cycle/editor.h"

#include "cycle/globals.h"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

void Editor::processInput()
{
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

void Editor::draw()
{
    assert(camera);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    updateGizmo();

    {
        ImGui::Begin("Entities");

        for (EntityID entity : g_entityManager->transformComponents.getEntities()) {
            TransformComponent *transformComponent = g_entityManager->transformComponents.getComponent(entity);
            NameComponent *nameComponent = g_entityManager->nameComponents.getComponent(entity);
            if (!transformComponent || !nameComponent)
                continue;

            if (ImGui::Selectable(nameComponent->name, entity == selectedEntityID)) {
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
        TransformComponent *transformComponent = g_entityManager->transformComponents.getComponent(selectedEntityID);
        if (!transformComponent)
            return;

        mat4 view = camera->getView();
        // NOTE: ImGuizmo does not support reverse depth perspective so recreate it
        mat4 projection = glm::perspective(camera->getFov(), camera->getAspectRatio(), camera->getZNear(), 1000.0f);

        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmoOperation, gizmoMode, glm::value_ptr(transformComponent->transform), NULL, NULL);
    }
}