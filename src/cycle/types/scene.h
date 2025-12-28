#pragma once

#include "cycle/containers.h"
#include "cycle/types/id.h"
#include "cycle/types/light.h"

class Scene
{
public:
    void addModel(ModelID modelID);
    void addLight(const Light &light);

private:
    Vector<ModelID> modelIDs;
    Vector<Light> lights;
};