#pragma once

#include "ImGuizmo.h"
#include "cycle/types/camera.h"
#include "cycle/types/id.h"

class Editor
{
public:
    static void init();
    static Editor *get();
    void processInput();
    void draw();

    void setCamera(Camera *camera) { this->camera = camera; }

    bool isMouseProcessed = true;
    bool isKeyboardProcessed = true;

private:
    Editor() {}
    Editor(Editor &) = delete;
    void operator=(Editor const &) = delete;

    void updateGizmo();

    ImGuizmo::OPERATION gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
    ImGuizmo::MODE gizmoMode = ImGuizmo::MODE::WORLD;

    EntityID selectedEntityID = EntityID::Invalid;
    Camera *camera = nullptr;

    bool lastFrameManipulated = false;
};