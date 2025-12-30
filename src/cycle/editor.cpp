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
}

void Editor::draw()
{
    assert(camera);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::BeginFrame();
    ImGuiIO &io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    World &world = g_engine->getWorld();

    Entity *selectedEntity = world.getEntityByID(selectedEntityID);
    if (selectedEntity) {
        mat4 view = camera->getView();
        // NOTE: ImGuizmo does not support reverse depth perspective so recreate it
        mat4 projection = glm::perspective(camera->getFov(), camera->getAspectRatio(), camera->getZNear(), 1000.0f);

        mat4 world = selectedEntity->transform.getMatrix();

        if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmoOperation, gizmoMode, glm::value_ptr(world), NULL, NULL)) {
            selectedEntity->transform.setMatrix(world);
        }
    }

    ImGui::ShowDemoWindow();

    ImGui::Render();
}