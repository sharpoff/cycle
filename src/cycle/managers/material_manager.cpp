#include "cycle/managers/material_manager.h"

#include "cycle/globals.h"

void MaterialManager::init()
{
    static MaterialManager materialManager;
    g_materialManager = &materialManager;
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