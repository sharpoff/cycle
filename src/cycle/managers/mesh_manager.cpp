#include "cycle/managers/mesh_manager.h"
#include "cycle/graphics/vulkan_types.h"

#include "cycle/globals.h"

void MeshManager::init(RenderDevice *renderDevice)
{
    static MeshManager meshManager;
    g_meshManager = &meshManager;

    meshManager.renderDevice = renderDevice;
}

void MeshManager::release()
{
    for (auto &mesh : meshes) {
        renderDevice->destroyBuffer(mesh.vertexBuffer);
        renderDevice->destroyBuffer(mesh.indexBuffer);
    }
}

const MeshID MeshManager::addMesh(Mesh &mesh)
{
    MeshID id = (MeshID)meshes.size();
    meshes.push_back(mesh);
    return id;
}

Mesh *MeshManager::getMeshByID(const MeshID id)
{
    if (id == MeshID::Invalid || (size_t)id >= meshes.size())
        return nullptr;

    return &meshes[(size_t)id];
}

void MeshManager::uploadMeshData(const MeshID &id, Vector<Vertex> vertices, Vector<uint32_t> indices)
{
    if (id == MeshID::Invalid || (size_t)id >= meshes.size())
        return;

    Mesh &mesh = meshes[(size_t)id];
    mesh.indices = indices;
    mesh.vertices = vertices;

    // vertex buffer
    {
        BufferCreateInfo createInfo = {
            .size = vertices.size() * sizeof(Vertex),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        };

        renderDevice->createBuffer(mesh.vertexBuffer, createInfo);
        renderDevice->uploadBufferData(mesh.vertexBuffer, vertices.data(), createInfo.size);
    }

    // index buffer
    {
        BufferCreateInfo createInfo = {
            .size = indices.size() * sizeof(uint32_t),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        renderDevice->createBuffer(mesh.indexBuffer, createInfo);
        renderDevice->uploadBufferData(mesh.indexBuffer, indices.data(), createInfo.size);
    }
}