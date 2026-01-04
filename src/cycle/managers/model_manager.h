#pragma once

#include "cycle/containers.h"
#include "cycle/math.h"
#include "cycle/types/id.h"
#include "cycle/types/model.h"

class ModelManager
{
public:
    static void init();

    const ModelID addModel(Model &model, String name = "");
    const ModelID loadModel(String filename, String name = "");

    Model *getModelByName(String name);
    Model *getModelByID(const ModelID id);
    const ModelID getModelIDByName(String name);

    vec3 getHalfExtent(const ModelID id, const vec3 &scale = vec3(1.0f));

    Vector<Model> &getModels() { return models; }

private:
    ModelManager() {}
    ModelManager(ModelManager &) = delete;
    void operator=(ModelManager const &) = delete;

    Vector<Model> models;
    UnorderedMap<String, ModelID> nameModelMap;
};