#pragma once

#include "cycle/containers.h"
#include "cycle/types/id.h"
#include "cycle/types/material.h"

class MaterialManager
{
public:
    static void init();
    const MaterialID addMaterial(const Material &material, String name = "");

    Vector<Material> &getMaterials() { return materials; }

private:
    Vector<Material> materials;
    UnorderedMap<String, MaterialID> nameMaterialMap;
};