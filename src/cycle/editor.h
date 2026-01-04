#pragma once

#include "ImGuizmo.h"
#include "cycle/types/camera.h"
#include "cycle/types/id.h"

class Editor
{
public:
    void processInput();
    void draw();

    // void selectEntity(const EntityID id) { selectedEntityID = id; }
    void setCamera(Camera *camera) { this->camera = camera; }

private:
    void updateGizmo();

    ImGuizmo::OPERATION gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE gizmoMode = ImGuizmo::MODE::WORLD;

    EntityID selectedEntityID = EntityID::Invalid;
    Camera *camera = nullptr;
};