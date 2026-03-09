#pragma once

#include "core/containers.h"
#include "types/id.h"
#include "types/material.h"

class MaterialCache
{
public:
    MaterialCache() = default;

    const MaterialID addMaterial(const Material &material, String name = "");

    Vector<Material> &getMaterials() { return materials; }

private:
    MaterialCache(MaterialCache &) = delete;
    void operator=(MaterialCache const &) = delete;

    Vector<Material> materials;
    UnorderedMap<String, MaterialID> nameMaterialMap;
};