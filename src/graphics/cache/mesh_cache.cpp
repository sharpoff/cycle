#include "graphics/cache/mesh_cache.h"

#include "graphics/render_device.h"

MeshCache::MeshCache(RenderDevice &renderDevice)
    : renderDevice(renderDevice)
{
}

void MeshCache::release()
{
    for (Mesh &mesh : meshes) {
        for (MeshPrimitive &prim : mesh.primitives) {
            renderDevice.destroyBuffer(prim.vertexBuffer);
            renderDevice.destroyBuffer(prim.indexBuffer);
        }
    }
}

const MeshID MeshCache::addMesh(Mesh &mesh, String name)
{
    if (MeshID id = getMeshIDByName(name); id != MeshID::Invalid)
        return id;

    MeshID id = (MeshID)meshes.size();
    meshes.push_back(std::move(mesh));
    if (!name.empty())
        nameMeshMap[name] = id;
    return id;
}

void MeshCache::uploadMeshGPUData(MeshPrimitive &prim)
{
    // vertex buffer
    {
        BufferCreateInfo createInfo = {
            .size = prim.vertices.size() * sizeof(Vertex),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        prim.vertexBuffer = renderDevice.createBuffer(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        renderDevice.uploadBufferData(prim.vertexBuffer, prim.vertices.data(), createInfo.size);
    }

    // index buffer
    {
        BufferCreateInfo createInfo = {
            .size = prim.indices.size() * sizeof(uint32_t),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        prim.indexBuffer = renderDevice.createBuffer(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        renderDevice.uploadBufferData(prim.indexBuffer, prim.indices.data(), createInfo.size);
    }
}


Mesh *MeshCache::getMeshByName(String name)
{
    MeshID id = getMeshIDByName(name);
    if (id != MeshID::Invalid)
        return &meshes[(size_t)id];

    return nullptr;
}

Mesh *MeshCache::getMeshByID(const MeshID id)
{
    if (id == MeshID::Invalid || (size_t)id >= meshes.size())
        return nullptr;

    return &meshes[(size_t)id];
}

const MeshID MeshCache::getMeshIDByName(String name)
{
    if (name.empty()) {
        return MeshID::Invalid;
    }

    auto it = nameMeshMap.find(name);
    if (it != nameMeshMap.end())
        return nameMeshMap[name];

    return MeshID::Invalid;
}

vec3 MeshCache::getHalfExtent(const MeshID id, const vec3 &scale)
{
    Mesh *mesh = getMeshByID(id);
    if (!mesh)
        return vec3();

    mat4 scaleMatrix = glm::scale(scale);

    vec3 min = vec3(std::numeric_limits<float>::max());
    vec3 max = vec3(std::numeric_limits<float>::min());

    for (MeshPrimitive &prim : mesh->primitives) {
        for (Vertex vert : prim.vertices) {
            vec3 position = vec3(scaleMatrix * vec4(vert.position, 0.0f));
            min = glm::min(min, position);
            max = glm::max(max, position);
        }
    }

    return (max - min) / 2.0f;
}