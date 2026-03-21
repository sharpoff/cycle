#pragma once

#include "core/containers.h"
#include "math/math_types.h"
#include "graphics/id.h"
#include "graphics/mesh.h"

class RenderDevice;

class MeshCache
{
public:
    MeshCache(RenderDevice &renderDevice);
    void release();

    const MeshID addMesh(Mesh &mesh, String name = "");
    void uploadMeshGPUData(MeshPrimitive &prim);

    Mesh *getMeshByName(String name);
    Mesh *getMeshByID(const MeshID id);
    const MeshID getMeshIDByName(String name);
    Vector<Mesh> &getMeshes() { return meshes; }

    vec3 getHalfExtent(const MeshID id, const vec3 &scale = vec3(1.0f));

private:
    MeshCache(MeshCache &) = delete;
    void operator=(MeshCache const &) = delete;

    Vector<Mesh> meshes;
    UnorderedMap<String, MeshID> nameMeshMap;

    RenderDevice &renderDevice;
};