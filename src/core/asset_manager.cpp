#include "core/asset_manager.h"
#include "core/logger.h"

#include "core/gltf_helpers.h"
#include "core/gltf_loader.h"
#include "graphics/renderer.h"

#include "ktx.h"
#include "stb_image.h"

void AssetManager::Initialize()
{
}

void AssetManager::Shutdown()
{
    // destroy textures
    for (auto &texture : textures)
        gRenderer->GetDevice().DestroyTexture(texture);

    // destroy mesh buffers
    for (auto &model : models) {
        for (auto &mesh : model.meshes) {
            for (auto &prim : mesh.primitives) {
                gRenderer->GetDevice().DestroyBuffer(prim.vertexBuffer);
                gRenderer->GetDevice().DestroyBuffer(prim.indexBuffer);
            }
        }
    }

    if (gAssetManager)
        delete gAssetManager;
}

uint32_t AssetManager::CreateModel(const FilePath &filename, const String &name)
{
    auto it = nameModelIndexMap.find(name);
    if (it != nameModelIndexMap.end()) {
        LOGW("model with name '{}' already exists, skipping...", name);
        return it->second;
    }

    Model model;
    if (filename.extension() == ".gltf" || filename.extension() == ".glb") {
        gltf::Scene scene;
        if (!gltf::Loader::Load(scene, filename))
            return InvalidIndex;

        if (!gltf::ConvertToModel(model, scene))
            return InvalidIndex;
    }

    models.push_back(model);
    return models.size() - 1;
}

uint32_t AssetManager::CreateTexture(const FilePath &file, const String &name)
{
    auto it = nameTextureIndexMap.find(name);
    if (it != nameTextureIndexMap.end()) {
        LOGW("texture with name '{}' already exists, skipping...", name);
        return it->second;
    }

    // check if compressed ktx image exists
    FilePath filename = file;
    auto ktxFile = filename;

    ktxFile.replace_extension(".ktx");
    FilePath ktxPath = ktxFile.parent_path() / "compressed" / ktxFile.filename();

    if (std::filesystem::exists(ktxPath))
        filename = ktxPath;

    TextureLoadInfo info = {};
    if (!LoadImageInfo(filename, info)) {
        LOGE("failed to load a texture from path '{}'", filename.string());
        return InvalidIndex;
    }

    const TextureCreateInfo createInfo = {
        .width = info.width,
        .height = info.height,
        .arrayLayers = info.arrayLayers,
        .mipLevels = info.mipLevels,
        .type = info.arrayLayers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .format = VK_FORMAT_R8G8B8A8_SRGB, // XXX: add this as a parameter or something
    };

    Texture texture;
    gRenderer->GetDevice().CreateTexture(texture, createInfo);
    gRenderer->GetDevice().UploadTexture(texture, info);
    FreeImageInfo(info);

    // generate mipmaps for non ktx images
    if (!info.textureKTX && createInfo.mipLevels > 1)
        gRenderer->GetDevice().GenerateMipmaps(texture);

    uint32_t id = textures.size();
    nameTextureIndexMap[name] = id;
    textures.push_back(texture);
    return id;
}

uint32_t AssetManager::CreateTexture(unsigned char *data, uint32_t size, const String &name)
{
    TextureLoadInfo info = {};
    if (!LoadImageInfo(data, size, info)) {
        LOGE("Failed to load a texture from memory");
        return InvalidIndex;
    }

    const TextureCreateInfo createInfo = {
        .width = info.width,
        .height = info.height,
        .arrayLayers = info.arrayLayers,
        .mipLevels = info.mipLevels,
        .type = info.arrayLayers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .format = VK_FORMAT_R8G8B8A8_SRGB, // XXX: add this as a parameter or something
    };

    Texture texture;
    gRenderer->GetDevice().CreateTexture(texture, createInfo);
    gRenderer->GetDevice().UploadTexture(texture, info);
    FreeImageInfo(info);

    // generate mipmaps for non ktx images
    if (!info.textureKTX && createInfo.mipLevels > 1)
        gRenderer->GetDevice().GenerateMipmaps(texture);

    uint32_t id = textures.size();
    nameTextureIndexMap[name] = id;
    textures.push_back(texture);
    return id;
}

uint32_t AssetManager::CreateMaterial(const String &name)
{
    auto it = nameMaterialIndexMap.find(name);
    if (it != nameMaterialIndexMap.end()) {
        LOGW("material with name '{}' already exists, skipping...", name);
        return it->second;
    }

    uint32_t id = materials.size();
    nameMaterialIndexMap[name] = id;
    materials.emplace_back();

    return id;
}

Model *AssetManager::GetModelByName(const String &name)
{
    auto it = nameModelIndexMap.find(name);
    if (it != nameModelIndexMap.end()) {
        return &models[it->second];
    }

    LOGW("model with name '{}' not found.", name);
    return nullptr;
}

Texture *AssetManager::GetTextureByName(const String &name)
{
    auto it = nameTextureIndexMap.find(name);
    if (it != nameTextureIndexMap.end()) {
        return &textures[it->second];
    }

    LOGW("texture with name '{}' not found.", name);
    return nullptr;
}

Material *AssetManager::GetMaterialByName(const String &name)
{
    auto it = nameMaterialIndexMap.find(name);
    if (it != nameMaterialIndexMap.end()) {
        return &materials[it->second];
    }

    LOGW("material with name '{}' not found.", name);
    return nullptr;
}

Model *AssetManager::GetModelByIndex(uint32_t index)
{
    if (index < models.size())
        return &models[index];

    return nullptr;
}

Texture *AssetManager::GetTextureByIndex(uint32_t index)
{
    if (index < textures.size())
        return &textures[index];

    return nullptr;
}

Material *AssetManager::GetMaterialByIndex(uint32_t index)
{
    if (index < materials.size())
        return &materials[index];

    return nullptr;
}

bool AssetManager::LoadImageInfo(const FilePath &filepath, TextureLoadInfo &info, bool flip)
{
    if (!std::filesystem::exists(filepath))
        return false;

    if (String(filepath).ends_with(".ktx")) { // compressed ktx image
        ktxResult result;

        ktxTexture *textureKTX = nullptr;
        result = ktxTexture_CreateFromNamedFile(filepath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &textureKTX);
        if (result != KTX_SUCCESS)
            return false;

        info.channels = 4; // rgba
        info.width = textureKTX->baseWidth;
        info.height = textureKTX->baseHeight;
        info.mipLevels = textureKTX->numLevels;
        info.arrayLayers = textureKTX->numFaces;
        info.data = ktxTexture_GetData(textureKTX);
        info.size = ktxTexture_GetDataSize(textureKTX);
        info.textureKTX = textureKTX;
    } else { // other image types

        int loadedChannels;
        info.data = stbi_load(filepath.c_str(), (int *)&info.width, (int *)&info.height, (int *)&loadedChannels, STBI_rgb_alpha);
        stbi_set_flip_vertically_on_load(flip);
        if (!info.data) {
            FreeImageInfo(info);
            return false;
        }

        info.channels = STBI_rgb_alpha;
        info.arrayLayers = 1;
        info.size = info.width * info.height * STBI_rgb_alpha;
        info.mipLevels = gRenderer->GetDevice().CalculateMipLevels(info.width, info.height);
    }

    return true;
}

bool AssetManager::LoadImageInfo(unsigned char *data, uint32_t size, TextureLoadInfo &info, bool flip)
{
    int loadedChannels;
    info.data = stbi_load_from_memory(data, size, (int *)&info.width, (int *)&info.height, (int *)&loadedChannels, STBI_rgb_alpha);
    stbi_set_flip_vertically_on_load(flip);
    if (!info.data) {
        FreeImageInfo(info);
        return false;
    }

    info.channels = STBI_rgb_alpha;
    info.arrayLayers = 1;
    info.size = info.width * info.height * STBI_rgb_alpha;
    info.mipLevels = gRenderer->GetDevice().CalculateMipLevels(info.width, info.height);

    return true;
}

void AssetManager::FreeImageInfo(TextureLoadInfo &info)
{
    if (info.textureKTX) {
        ktxTexture_Destroy(info.textureKTX);
    } else {
        stbi_image_free(info.data);
    }
}