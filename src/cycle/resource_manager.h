#pragma once

#include "cycle/types/id.h"
#include "cycle/containers.h"

#include "cycle/graphics/graphics_types.h"
#include "cycle/types/entity.h"
#include "cycle/types/material.h"
#include "cycle/types/mesh.h"
#include "cycle/types/model.h"

// TODO: remove resource functions

class RenderDevice;

class ResourceManager
{
public:
    void setRenderDevice(RenderDevice *renderDevice);
    void destroyAllResources();

    ModelID   loadModelFromFile(String name, String filename);
    TextureID loadTextureFromFile(String name, String filename);

    void uploadMeshBuffers(Mesh &mesh);

    EntityID   addEntity(String name, const Entity &entity);
    ModelID    addModel(String name, const Model &model);
    MeshID     addMesh(String name, const Mesh &mesh);
    TextureID  addTexture(String name, Image &image);
    MaterialID addMaterial(String name, const Material &material);

    Entity   *getEntityByName(String name);
    Model    *getModelByName(String name);
    Mesh     *getMeshByName(String name);
    Image    *getTextureByName(String name);
    Material *getMaterialByName(String name);

    Entity   *getEntityByID(EntityID id);
    Model    *getModelByID(ModelID id);
    Mesh     *getMeshByID(MeshID id);
    Image    *getTextureByID(TextureID id);
    Material *getMaterialByID(MaterialID id);

    String getUniqueEntityName(String baseName = "");
    String getUniqueModelName(String baseName = "");
    String getUniqueMeshName(String baseName = "");
    String getUniqueTextureName(String baseName = "");
    String getUniqueMaterialName(String baseName = "");

    Vector<Entity>   &getEntities() { return entities; }
    Vector<Model>    &getModels() { return models; }
    Vector<Mesh>     &getMeshes() { return meshes; }
    Vector<Image>    &getTextures() { return textures; }
    Vector<Material> &getMaterials() { return materials; }

private:
    Vector<Entity>   entities;
    Vector<Model>    models;
    Vector<Mesh>     meshes;
    Vector<Image>    textures;
    Vector<Material> materials;

    UnorderedMap<String, EntityID>   entityNameMap;
    UnorderedMap<String, ModelID>    modelNameMap;
    UnorderedMap<String, MeshID>     meshNameMap;
    UnorderedMap<String, TextureID>  textureNameMap;
    UnorderedMap<String, MaterialID> materialNameMap;

    RenderDevice *renderDevice;
};