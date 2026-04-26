#pragma once

#include "graphics/material.h"
#include "graphics/model.h"
#include "graphics/vulkan_types.h"

class AssetManager
{
public:
    friend class Engine;

    void Initialize();
    void Shutdown();

    uint32_t CreateModel(const FilePath &filename, const String &name);
    uint32_t CreateTexture(const FilePath &filename, const String &name);
    uint32_t CreateTexture(unsigned char *data, uint32_t size, const String &name);
    uint32_t CreateMaterial(const String &name);

    Model *GetModelByName(const String &name);
    Texture *GetTextureByName(const String &name);
    Material *GetMaterialByName(const String &name);

    Model *GetModelByIndex(uint32_t index);
    Texture *GetTextureByIndex(uint32_t index);
    Material *GetMaterialByIndex(uint32_t index);

    Vector<Model> &GetModels() { return models; }
    Vector<Texture> &GetTextures() { return textures; }
    Vector<Material> &GetMaterials() { return materials; }

private:
    AssetManager() {}
    AssetManager(const AssetManager &) = delete;
    AssetManager(AssetManager &&) = delete;
    AssetManager &operator=(const AssetManager &) = delete;
    AssetManager &operator=(AssetManager &&) = delete;

    bool LoadImageInfo(const FilePath &filepath, TextureLoadInfo &info, bool flip = false);
    bool LoadImageInfo(unsigned char *data, uint32_t size, TextureLoadInfo &info, bool flip = false);
    void FreeImageInfo(TextureLoadInfo &info);

    Vector<Model> models;
    Vector<Texture> textures;
    Vector<Material> materials;

    UnorderedMap<String, uint32_t> nameModelIndexMap;
    UnorderedMap<String, uint32_t> nameTextureIndexMap;
    UnorderedMap<String, uint32_t> nameMaterialIndexMap;
};

inline AssetManager *gAssetManager = nullptr;