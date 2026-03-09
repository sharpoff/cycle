#include "graphics/cache/material_cache.h"

const MaterialID MaterialCache::addMaterial(const Material &material, String name)
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