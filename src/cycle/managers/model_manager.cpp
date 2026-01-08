#include "cycle/managers/model_manager.h"
#include "cycle/gltf_loader.h"
#include "cycle/logger.h"

#include "cycle/graphics/render_device.h"

ModelManager *g_modelManager = nullptr;

void ModelManager::init(RenderDevice *renderDevice)
{
    static ModelManager instance;
    g_modelManager = &instance;

    instance.renderDevice = renderDevice;
}

void ModelManager::release()
{
    for (Model &model : models) {
        for (Mesh &mesh : model.meshes) {
            renderDevice->destroyBuffer(mesh.vertexBuffer);
            renderDevice->destroyBuffer(mesh.indexBuffer);
        }
    }
}

void ModelManager::uploadMeshData(Mesh &mesh, Vector<Vertex> &vertices, Vector<uint32_t> &indices)
{
    mesh.vertices = vertices;
    mesh.indices = indices;

    // vertex buffer
    {
        BufferCreateInfo createInfo = {
            .size = mesh.vertices.size() * sizeof(Vertex),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        };

        renderDevice->createBuffer(mesh.vertexBuffer, createInfo);
        renderDevice->uploadBufferData(mesh.vertexBuffer, vertices.data(), createInfo.size);
    }

    // index buffer
    {
        BufferCreateInfo createInfo = {
            .size = mesh.indices.size() * sizeof(uint32_t),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        renderDevice->createBuffer(mesh.indexBuffer, createInfo);
        renderDevice->uploadBufferData(mesh.indexBuffer, indices.data(), createInfo.size);
    }
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

    for (Mesh &mesh : model->meshes) {
        for (Vertex vert : mesh.vertices) {
            vec3 position = vec3(scaleMatrix * vec4(vert.position, 0.0f));
            min = glm::min(min, position);
            max = glm::max(max, position);
        }
    }

    return (max - min) / 2.0f;
}