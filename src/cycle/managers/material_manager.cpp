#include "cycle/managers/material_manager.h"

MaterialManager *g_materialManager = nullptr;

void MaterialManager::init()
{
    static MaterialManager instance;
    g_materialManager = &instance;
}

const MaterialID MaterialManager::addMaterial(const Material &material, String name)
{
    if (!name.empty()) {
        auto it = nameMaterialMap.find(name);
        if (it != nameMaterialMap.end()) {
            return it->second;
        }
    }

    MaterialID id = (MaterialID)materials.size();
    materials.push_back(material);
    if (!name.empty())
        nameMaterialMap[name] = id;
    return id;
}