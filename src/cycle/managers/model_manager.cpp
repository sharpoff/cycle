#include "cycle/managers/model_manager.h"
#include "cycle/gltf_loader.h"
#include "cycle/logger.h"

#include "cycle/globals.h"

void ModelManager::init()
{
    static ModelManager modelManager;
    g_modelManager = &modelManager;
}

const ModelID ModelManager::addModel(Model &model, String name)
{
    if (ModelID id = getModelIDByName(name); id != ModelID::Invalid) {
        return id;
    }

    ModelID id = (ModelID)models.size();
    models.push_back(model);
    if (!name.empty())
        nameModelMap[name] = id;
    return id;
}

const ModelID ModelManager::loadModel(String filename, String name)
{
    Model model = {};
    if (!gltf::load(model, filename)) {
        LOGE("Failed to load model from '%s'", filename.c_str());
        return ModelID::Invalid;
    }

    return addModel(model, name);
}

Model *ModelManager::getModelByName(String name)
{
    ModelID id = getModelIDByName(name);
    if (id != ModelID::Invalid)
        return &models[(size_t)id];

    return nullptr;
}

Model *ModelManager::getModelByID(const ModelID id)
{
    if (id == ModelID::Invalid || (size_t)id >= models.size())
        return nullptr;

    return &models[(size_t)id];
}

const ModelID ModelManager::getModelIDByName(String name)
{
    if (name.empty()) {
        return ModelID::Invalid;
    }

    auto it = nameModelMap.find(name);
    if (it != nameModelMap.end())
        return nameModelMap[name];

    return ModelID::Invalid;
}

vec3 ModelManager::getHalfExtent(const ModelID id, const vec3 &scale)
{
    Model *model = getModelByID(id);
    if (!model)
        return vec3();

    mat4 scaleMatrix = glm::scale(scale);

    vec3 min = vec3(std::numeric_limits<float>::max());
    vec3 max = vec3(std::numeric_limits<float>::min());

    for (MeshID meshID : model->meshes) {
        Mesh *mesh = g_meshManager->getMeshByID(meshID);
        if (!mesh)
            return vec3();

        if (mesh->vertices.size() == 0)
            return vec3();

        for (Vertex vert : mesh->vertices) {
            vec3 position = vec3(scaleMatrix * vec4(vert.position, 0.0f));
            min = glm::min(min, position);
            max = glm::max(max, position);
        }
    }

    return (max - min) / 2.0f;
}