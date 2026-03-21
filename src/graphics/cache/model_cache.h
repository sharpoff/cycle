#pragma once

#include "core/containers.h"
#include "graphics/id.h"
#include "graphics/model.h"
#include <filesystem>

class Renderer;

class ModelCache
{
public:
    ModelCache(Renderer &renderer);

    const ModelID addModel(const Model &model, String name = "");
    const ModelID loadFromFile(std::filesystem::path filename, String name);

    Model *getModel(String name);
    Model *getModel(const ModelID id);

    Vector<Model> &getModels() { return models; }

private:
    ModelCache(ModelCache &) = delete;
    void operator=(ModelCache const &) = delete;

    Vector<Model> models;
    UnorderedMap<String, ModelID> nameModelMap;
    UnorderedMap<std::filesystem::path, ModelID> filenameModelIDMap;

    Renderer &renderer;
};