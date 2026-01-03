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

void Editor::draw(World &world)
{
    assert(camera);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Gizmo
    {
        ImGuizmo::BeginFrame();
        ImGuiIO &io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        Entity *selectedEntity = world.getEntityByID(selectedEntityID);
        if (selectedEntity) {
            mat4 view = camera->getView();
            // NOTE: ImGuizmo does not support reverse depth perspective so recreate it
            mat4 projection = glm::perspective(camera->getFov(), camera->getAspectRatio(), camera->getZNear(), 1000.0f);

            mat4 worldMatrix = selectedEntity->transform.getMatrix();

            if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), gizmoOperation, gizmoMode, glm::value_ptr(worldMatrix), NULL, NULL)) {
                selectedEntity->transform.setMatrix(worldMatrix);
            }
        }
    }

    {
        ImGui::Begin("World");

        std::vector<Entity*> &entities = world.getEntities();
        for (size_t i = 0; i < entities.size(); i++) {
            Entity *entity = entities[i];
            bool selected = entity == world.getEntityByID(selectedEntityID);
            if (ImGui::Selectable(entity->name.c_str(), selected)) {
                selectedEntityID = selected ? EntityID::Invalid : EntityID(i);
            }
        }

        ImGui::End();
    }

    if (selectedEntityID != EntityID::Invalid)
    {
        ImGui::Begin("Properties");

        Entity *selectedEntity = world.getEntityByID(selectedEntityID);
        vec3 position = selectedEntity->transform.getPosition();
        vec3 scale = selectedEntity->transform.getScale();
        if (ImGui::DragFloat3("position", glm::value_ptr(position), 0.25f, -1000.0f, 1000.0f)) {
            selectedEntity->transform.setPosition(position);
        }
        if (ImGui::DragFloat3("scale", glm::value_ptr(scale), 0.25f, 0.01f, 1000.0f)) {
            selectedEntity->transform.setScale(scale);
        }

        // RenderingFlags renderingFlags;
        // ModelID modelID;
        // EntityType type = ENTITY_TYPE_NONE;

        ImGui::End();
    }

    ImGui::ShowDemoWindow();

    ImGui::Render();
}