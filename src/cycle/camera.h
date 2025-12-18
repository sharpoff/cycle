#pragma once

#include "cycle/math.h"

class Camera
{
public:
    Camera();

    void move(vec3 translation);
    void rotate(vec3 rotation);

    void setPosition(vec3 position);
    void setRotation(vec3 rotation);
    void setProjection(mat4 projection);

    mat4 getProjection() { return projection; }
    mat4 getView() { return view; }
    mat4 getRotation();

private:
    void updateView();

    mat4 projection = mat4(1.0f);
    mat4 view = mat4(1.0f);

    vec3 position = vec3(0.0f);
    vec3 rotation = vec3(0.0f);
};