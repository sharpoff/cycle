#include "cycle/resource_manager.h"
#include "cycle/graphics/render_device.h"
#include "cycle/id_types.h"

#include "cycle/gltf_loader.h"
#include "stb_image.h"

void ResourceManager::init(RenderDevice *renderDevice)
{
    assert(renderDevice);
    this->renderDevice = renderDevice;
}

void ResourceManager::destroyAllResources()
{
    for (Image &texture : textures) {
        renderDevice->destroyImage(texture);
    }

    for (Model &model : models) {
        for (MeshID meshID : model.meshes) {
            if (meshID == MeshID::Invalid)
                continue;

            Mesh *mesh = getMeshByID(meshID);
            if (!mesh)
                continue;

            renderDevice->destroyBuffer(mesh->vertexBuffer);
            renderDevice->destroyBuffer(mesh->indexBuffer);
        }
    }
}

ModelID ResourceManager::loadModelFromFile(String name, String filename)
{
    Model model;
    if (gltf::loadModel(this, model, filename)) {
        return addModel(name, model);
    }

    return ModelID::Invalid;
}

TextureID ResourceManager::loadTextureFromFile(String name, String filename)
{
    uint32_t width, height, channels;
    unsigned char *pixels = stbi_load(filename.c_str(), (int *)&width, (int *)&height, (int *)&channels, STBI_rgb_alpha);
    if (!pixels)
        return TextureID::Invalid;

    const ImageCreateInfo createInfo = {
        .width = width,
        .height = height,
        .usage = IMAGE_USAGE_SAMPLED | IMAGE_USAGE_TRANSFER_DST,
        .format = IMAGE_FORMAT_R8G8B8A8_UNORM,
    };

    Image image;
    renderDevice->createImage(image, createInfo, name);

    renderDevice->uploadImageData(image, pixels, width * height * STBI_rgb_alpha);
    stbi_image_free(pixels);

    return addTexture(name, image);
}

void ResourceManager::uploadMeshBuffers(Mesh &mesh)
{
    // vertices
    {
        const BufferCreateInfo createInfo = {
            .size = mesh.vertices.size() * sizeof(Vertex),
            .usage = BUFFER_USAGE_STORAGE | BUFFER_USAGE_DEVICE_ADDRESS | BUFFER_USAGE_TRANSFER_DST,
        };
        renderDevice->createBuffer(mesh.vertexBuffer, createInfo);

        renderDevice->uploadBufferData(mesh.vertexBuffer, mesh.vertices.data(), createInfo.size);
    }

    // indices
    {
        const BufferCreateInfo createInfo = {
            .size = mesh.indices.size() * sizeof(uint32_t),
            .usage = BUFFER_USAGE_INDEX | BUFFER_USAGE_DEVICE_ADDRESS | BUFFER_USAGE_TRANSFER_DST,
        };
        renderDevice->createBuffer(mesh.indexBuffer, createInfo);

        renderDevice->uploadBufferData(mesh.indexBuffer, mesh.indices.data(), createInfo.size);
    }
}

EntityID ResourceManager::addEntity(String name, const Entity &entity)
{
    auto it = entityNameMap.find(name);
    if (it == entityNameMap.end()) {
        auto id = EntityID(entities.size());
        entityNameMap[name] = id;
        entities.push_back(entity);
        return id;
    } else {
        return it->second;
    }
}

ModelID ResourceManager::addModel(String name, const Model &model)
{
    auto it = modelNameMap.find(name);
    if (it == modelNameMap.end()) {
        auto id = ModelID(models.size());
        modelNameMap[name] = id;
        models.push_back(model);
        return id;
    } else {
        return it->second;
    }
}

MeshID ResourceManager::addMesh(String name, const Mesh &mesh)
{
    auto it = meshNameMap.find(name);
    if (it == meshNameMap.end()) {
        auto id = MeshID(meshes.size());
        meshNameMap[name] = id;
        meshes.push_back(mesh);
        return id;
    } else {
        return it->second;
    }
}

TextureID ResourceManager::addTexture(String name, Image &image)
{
    auto it = textureNameMap.find(name);
    if (it == textureNameMap.end()) {
        auto id = TextureID(textures.size());
        textureNameMap[name] = id;
        textures.push_back(image);
        return id;
    } else {
        return it->second;
    }
}

MaterialID ResourceManager::addMaterial(String name, const Material &material)
{
    auto it = materialNameMap.find(name);
    if (it == materialNameMap.end()) {
        auto id = MaterialID(materials.size());
        materialNameMap[name] = id;
        materials.push_back(material);
        return id;
    } else {
        return it->second;
    }
}

Entity *ResourceManager::getEntityByName(String name)
{
    auto it = entityNameMap.find(name);
    if (it != entityNameMap.end()) {
        return &entities[(uint32_t)it->second];
    }

    return nullptr;
}

Model *ResourceManager::getModelByName(String name)
{
    auto it = modelNameMap.find(name);
    if (it != modelNameMap.end()) {
        return &models[(uint32_t)it->second];
    }

    return nullptr;
}

Mesh *ResourceManager::getMeshByName(String name)
{
    auto it = meshNameMap.find(name);
    if (it != meshNameMap.end()) {
        return &meshes[(uint32_t)it->second];
    }

    return nullptr;
}

Image *ResourceManager::getTextureByName(String name)
{
    auto it = textureNameMap.find(name);
    if (it != textureNameMap.end()) {
        return &textures[(uint32_t)it->second];
    }

    return nullptr;
}

Material *ResourceManager::getMaterialByName(String name)
{
    auto it = materialNameMap.find(name);
    if (it != materialNameMap.end()) {
        return &materials[(uint32_t)it->second];
    }

    return nullptr;
}

Entity *ResourceManager::getEntityByID(EntityID id)
{
    if (id == EntityID::Invalid)
        return nullptr;

    uint32_t idInt = uint32_t(id);
    if (idInt >= entities.size()) // out of bounds
        return nullptr;

    return &entities[idInt];
}

Model *ResourceManager::getModelByID(ModelID id)
{
    if (id == ModelID::Invalid)
        return nullptr;

    uint32_t idInt = uint32_t(id);
    if (idInt >= models.size()) // out of bounds
        return nullptr;

    return &models[idInt];
}

Mesh *ResourceManager::getMeshByID(MeshID id)
{
    if (id == MeshID::Invalid)
        return nullptr;

    uint32_t idInt = uint32_t(id);
    if (idInt >= meshes.size()) // out of bounds
        return nullptr;

    return &meshes[idInt];
}

Image *ResourceManager::getTextureByID(TextureID id)
{
    if (id == TextureID::Invalid)
        return nullptr;

    uint32_t idInt = uint32_t(id);
    if (idInt >= textures.size()) // out of bounds
        return nullptr;

    return &textures[idInt];
}

Material *ResourceManager::getMaterialByID(MaterialID id)
{
    if (id == MaterialID::Invalid)
        return nullptr;

    uint32_t idInt = uint32_t(id);
    if (idInt >= materials.size()) // out of bounds
        return nullptr;

    return &materials[idInt];
}

String ResourceManager::getUniqueEntityName(String baseName)
{
    size_t id = 0;
    for (; id <= entities.size(); id++) {
        std::string name = baseName + std::to_string(id);
        if (!getEntityByName(name)) {
            return name;
        }
    }

    return baseName + std::to_string(id);
}

String ResourceManager::getUniqueModelName(String baseName)
{
    size_t id = 0;
    for (; id <= models.size(); id++) {
        std::string name = baseName + std::to_string(id);
        if (!getModelByName(name)) {
            return name;
        }
    }

    return baseName + std::to_string(id);
}

String ResourceManager::getUniqueMeshName(String baseName)
{
    size_t id = 0;
    for (; id <= meshes.size(); id++) {
        std::string name = baseName + std::to_string(id);
        if (!getMeshByName(name)) {
            return name;
        }
    }

    return baseName + std::to_string(id);
}

String ResourceManager::getUniqueTextureName(String baseName)
{
    size_t id = 0;
    for (; id <= textures.size(); id++) {
        std::string name = baseName + std::to_string(id);
        if (!getTextureByName(name)) {
            return name;
        }
    }

    return baseName + std::to_string(id);
}

String ResourceManager::getUniqueMaterialName(String baseName)
{
    size_t id = 0;
    for (; id <= materials.size(); id++) {
        std::string name = baseName + std::to_string(id);
        if (!getMaterialByName(name)) {
            return name;
        }
    }

    return baseName + std::to_string(id);
}