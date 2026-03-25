#pragma once

#include "core/filesystem.h"
#include "graphics/material.h"
#include "graphics/model.h"
#include "graphics/vulkan_types.h"
#include <memory>

using ModelPtr = std::shared_ptr<Model>;
using TexturePtr = std::shared_ptr<Texture>;
using MaterialPtr = std::shared_ptr<Material>;

class AssetManager
{
public:
    friend class Application;

    void Init();
    void Shutdown();

    ModelPtr CreateModel(FilePath filename, String name);
    TexturePtr CreateTexture(FilePath filename, String name);
    TexturePtr CreateTexture(unsigned char *data, uint32_t size, String name);
    MaterialPtr CreateMaterial(String name);

    void RemoveModel(ModelPtr model);
    void RemoveModel(String name);

    void RemoveTexture(TexturePtr texture);
    void RemoveTexture(String name);

    void RemoveMaterial(MaterialPtr material);
    void RemoveMaterial(String name);

    ModelPtr GetModel(String name);
    TexturePtr GetTexture(String name);
    MaterialPtr GetMaterial(String name);

    Vector<ModelPtr> &GetModels() { return models; }
    Vector<TexturePtr> &GetTextures() { return textures; }
    Vector<MaterialPtr> &GetMaterials() { return materials; }

private:
    AssetManager() {}
    AssetManager(const AssetManager &) = delete;
    AssetManager(AssetManager &&) = delete;
    AssetManager &operator=(const AssetManager &) = delete;
    AssetManager &operator=(AssetManager &&) = delete;

    bool LoadImageInfo(FilePath filepath, TextureLoadInfo &info, bool flip = false);
    bool LoadImageInfo(unsigned char *data, uint32_t size, TextureLoadInfo &info, bool flip = false);
    void FreeImageInfo(TextureLoadInfo &info);

    Vector<ModelPtr> models;
    UnorderedMap<String, uint32_t> nameModelIdxMap;

    Vector<TexturePtr> textures;
    UnorderedMap<String, uint32_t> nameTextureIdxMap;

    Vector<MaterialPtr> materials;
    UnorderedMap<String, uint32_t> nameMaterialIdxMap;
};

static AssetManager *gAssetManager = nullptr;