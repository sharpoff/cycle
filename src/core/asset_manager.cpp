#include "core/asset_manager.h"
#include "core/logger.h"

#include "core/gltf_helpers.h"
#include "core/gltf_loader.h"
#include "graphics/renderer.h"

#include "ktx.h"
#include "stb_image.h"
#include <algorithm>

void AssetManager::Init()
{
}

void AssetManager::Shutdown()
{
    delete gAssetManager;
}

Model *AssetManager::CreateModel(FilePath filename, String name)
{
    auto it = nameModelIdxMap.find(name);
    if (it != nameModelIdxMap.end()) {
        LOGW("createModel() - model with name '%s' already exists, skipping...", name.c_str());
        return models[it->second];
    }

    Model *model = new Model();
    if (filename.extension() == ".gltf" || filename.extension() == ".glb") {
        gltf::Scene scene;
        if (!gltf::Loader::Load(scene, filename))
            return nullptr;

        if (!gltf::ConvertToModel(model, scene))
            return nullptr;
    }

    models.push_back(model);
    nameModelIdxMap[name] = models.size() - 1;
    return model;
}

Texture *AssetManager::CreateTexture(FilePath file, String name)
{
    auto it = nameTextureIdxMap.find(name);
    if (it != nameTextureIdxMap.end()) {
        LOGW("createTexture() - model with name '%s' already exists, skipping...", name.c_str());
        return textures[it->second];
    }

    // check if compressed ktx image exists
    auto ktxFile = file;
    ktxFile.replace_extension(".ktx");
    FilePath ktxPath = ktxFile.parent_path() / "compressed" / ktxFile.filename();

    if (std::filesystem::exists(ktxPath))
        file = ktxPath;

    TextureLoadInfo info = {};
    if (!LoadImageInfo(file, info)) {
        LOGE("createTexture() - failed to load a texture from path '%s'", file.c_str());
        return nullptr;
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

    Texture *texture = gRenderer->GetDevice().CreateTexture(createInfo);
    gRenderer->GetDevice().UploadTexture(texture, info);
    FreeImageInfo(info);

    // generate mipmaps for non ktx images
    if (!info.textureKTX && createInfo.mipLevels > 1)
        gRenderer->GetDevice().GenerateMipmaps(texture);

    textures.push_back(texture);
    nameTextureIdxMap[name] = textures.size() - 1;
    return texture;
}

Texture *AssetManager::CreateTexture(unsigned char *data, uint32_t size, String name)
{
    TextureLoadInfo info = {};
    if (!LoadImageInfo(data, size, info)) {
        LOGE("%s", "Failed to load a texture from memory");
        return nullptr;
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

    Texture *texture = gRenderer->GetDevice().CreateTexture(createInfo);
    gRenderer->GetDevice().UploadTexture(texture, info);
    FreeImageInfo(info);

    // generate mipmaps for non ktx images
    if (!info.textureKTX && createInfo.mipLevels > 1)
        gRenderer->GetDevice().GenerateMipmaps(texture);

    textures.push_back(texture);
    nameTextureIdxMap[name] = textures.size() - 1;
    return texture;
}

Material *AssetManager::CreateMaterial(String name)
{
    auto it = nameMaterialIdxMap.find(name);
    if (it != nameMaterialIdxMap.end()) {
        LOGW("createMaterial() - model with name '%s' already exists, skipping...", name.c_str());
        return materials[it->second];
    }

    Material *material = materials.emplace_back(new Material());
    nameMaterialIdxMap[name] = materials.size() - 1;
    return material;
}

void AssetManager::RemoveModel(Model *model)
{
    models.erase(std::remove_if(models.begin(), models.end(), [&model](const Model *otherModel) {
        return &model == &otherModel;
    }),
    models.end());
}

void AssetManager::RemoveModel(String name)
{
    Model *model = GetModel(name);
    if (model)
        RemoveModel(model);
}

void AssetManager::RemoveTexture(Texture *texture)
{
    textures.erase(std::remove_if(textures.begin(), textures.end(), [&texture](const Texture *otherTexture) {
        return &texture == &otherTexture;
    }),
    textures.end());
}

void AssetManager::RemoveTexture(String name)
{
    Texture *texture = GetTexture(name);
    if (texture)
        RemoveTexture(texture);
}

void AssetManager::RemoveMaterial(Material *material)
{
    materials.erase(std::remove_if(materials.begin(), materials.end(), [&material](const Material *otherMaterial) {
        return &material == &otherMaterial;
    }),
    materials.end());
}

void AssetManager::RemoveMaterial(String name)
{
    Material *material = GetMaterial(name);
    if (material)
        RemoveMaterial(material);
}

Model *AssetManager::GetModel(String name)
{
    auto it = nameModelIdxMap.find(name);
    if (it != nameModelIdxMap.end()) {
        return models[it->second];
    }

    LOGW("getModel() - model with name '%s' not found.", name.c_str());
    return nullptr;
}

Texture *AssetManager::GetTexture(String name)
{
    auto it = nameTextureIdxMap.find(name);
    if (it != nameTextureIdxMap.end()) {
        return textures[it->second];
    }

    LOGW("getTexture() - texture with name '%s' not found.", name.c_str());
    return nullptr;
}

Material *AssetManager::GetMaterial(String name)
{
    auto it = nameMaterialIdxMap.find(name);
    if (it != nameMaterialIdxMap.end()) {
        return materials[it->second];
    }

    LOGW("getMaterial() - material with name '%s' not found.", name.c_str());
    return nullptr;
}

bool AssetManager::LoadImageInfo(FilePath filepath, TextureLoadInfo &info, bool flip)
{
    if (!std::filesystem::exists(filepath))
        return false;

    if (String(filepath).ends_with(".ktx")) { // compressed ktx image
        // LOGI("Loading KTX texture: %s", filepath.c_str());
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
        // LOGI("Loading regular texture: %s", filepath.c_str());

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