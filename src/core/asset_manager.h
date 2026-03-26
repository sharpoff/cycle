#pragma once

#include "core/filesystem.h"
#include "graphics/material.h"
#include "graphics/model.h"
#include "graphics/vulkan_types.h"

class AssetManager
{
public:
    friend class Application;

    void Init();
    void Shutdown();

    Model *CreateModel(FilePath filename, String name);
    Texture *CreateTexture(FilePath filename, String name);
    Texture *CreateTexture(unsigned char *data, uint32_t size, String name);
    Material *CreateMaterial(String name);

    void RemoveModel(Model *model);
    void RemoveModel(String name);

    void RemoveTexture(Texture *texture);
    void RemoveTexture(String name);

    void RemoveMaterial(Material *material);
    void RemoveMaterial(String name);

    Model *GetModel(String name);
    Texture *GetTexture(String name);
    Material *GetMaterial(String name);

    Vector<Model *> &GetModels() { return models; }
    Vector<Texture *> &GetTextures() { return textures; }
    Vector<Material *> &GetMaterials() { return materials; }

private:
    AssetManager() {}
    AssetManager(const AssetManager &) = delete;
    AssetManager(AssetManager &&) = delete;
    AssetManager &operator=(const AssetManager &) = delete;
    AssetManager &operator=(AssetManager &&) = delete;

    bool LoadImageInfo(FilePath filepath, TextureLoadInfo &info, bool flip = false);
    bool LoadImageInfo(unsigned char *data, uint32_t size, TextureLoadInfo &info, bool flip = false);
    void FreeImageInfo(TextureLoadInfo &info);

    Vector<Model *> models;
    UnorderedMap<String, uint32_t> nameModelIdxMap;

    Vector<Texture *> textures;
    UnorderedMap<String, uint32_t> nameTextureIdxMap;

    Vector<Material *> materials;
    UnorderedMap<String, uint32_t> nameMaterialIdxMap;
};

inline AssetManager *gAssetManager = nullptr;