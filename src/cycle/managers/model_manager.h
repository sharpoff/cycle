#pragma once

#include "cycle/containers.h"
#include "cycle/math.h"
#include "cycle/types/id.h"
#include "cycle/types/model.h"

class RenderDevice;

class ModelManager
{
public:
    static void init(RenderDevice *renderDevice);
    void release();

    const ModelID addModel(Model &model, String name = "");
    const ModelID loadModel(String filename, String name = "");

    void uploadMeshData(Mesh &mesh, Vector<Vertex> &vertices, Vector<uint32_t> &indices);

    Model *getModelByName(String name);
    Model *getModelByID(const ModelID id);
    const ModelID getModelIDByName(String name);
    Vector<Model> &getModels() { return models; }

    vec3 getHalfExtent(const ModelID id, const vec3 &scale = vec3(1.0f));

private:

    ModelManager() {}
    ModelManager(ModelManager &) = delete;
    void operator=(ModelManager const &) = delete;

    Vector<Model> models;

    RenderDevice *renderDevice;
    UnorderedMap<String, ModelID> nameModelMap;
};