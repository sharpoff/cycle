#pragma once

#include "cycle/graphics/render_device.h"
#include "cycle/types/id.h"
#include "cycle/types/mesh.h"

class MeshManager
{
public:
    static void init(RenderDevice *renderDevice);
    void release();

    const MeshID addMesh(Mesh &mesh);

    Mesh *getMeshByID(const MeshID id);

    void uploadMeshData(const MeshID &id, Vector<Vertex> vertices, Vector<uint32_t> indices);

private:
    Vector<Mesh> meshes;

    RenderDevice *renderDevice;
};