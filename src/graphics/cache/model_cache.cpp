#include "graphics/cache/model_cache.h"
#include "core/gltf_helpers.h"
#include "core/gltf_loader.h"

#include "graphics/renderer.h"

ModelCache::ModelCache(Renderer &renderer)
    : renderer(renderer)
{
}

const ModelID ModelCache::addModel(const Model &model, String name)
{
    if (!name.empty()) {
        auto it = nameModelMap.find(name);
        if (it != nameModelMap.end()) {
            return it->second;
        }
    }

    ModelID id = (ModelID)models.size();
    models.push_back(std::move(model));
    if (!name.empty())
        nameModelMap[name] = id;
    return id;
}

const ModelID ModelCache::loadFromFile(std::filesystem::path filename, String name)
{
    auto it = filenameModelIDMap.find(filename);
    if (it != filenameModelIDMap.end())
        return it->second;

    auto it2 = nameModelMap.find(name);
    if (it2 != nameModelMap.end())
        return it2->second;

    Model model;
    if (filename.extension() == ".gltf" || filename.extension() == ".glb") {
        gltf::Scene scene;
        if (!gltf::Loader::load(renderer.getCacheManager(), scene, filename))
            return ModelID::Invalid;

        if (!gltf::convertToModel(model, scene))
            return ModelID::Invalid;
    }

    // loaded successfully
    if (!model.meshIDs.empty()) {
        const ModelID id = addModel(std::move(model), name);
        filenameModelIDMap[name] = id;
        return id;
    }

    return ModelID::Invalid;
}

Model *ModelCache::getModel(String name)
{
    auto it = nameModelMap.find(name);
    if (it != nameModelMap.end())
        return &models[(uint32_t)it->second];

    return nullptr;
}

Model *ModelCache::getModel(const ModelID id)
{
    if (id != ModelID::Invalid && (uint32_t)id < models.size())
        return &models[(uint32_t)id];

    return nullptr;
}