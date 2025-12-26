#include "cycle/gltf_loader.h"

#include "cgltf.h"
#include "cycle/id_types.h"
#include "cycle/types/material.h"
#include "cycle/types/mesh.h"

#include "cycle/resource_manager.h"

namespace gltf
{
    static void processNode(ResourceManager *resourceManager, Model &model, cgltf_data *data, cgltf_node *gltfNode);

    bool loadModel(ResourceManager *resourceManager, Model &model, const String &filename)
    {
        cgltf_options options = {};
        cgltf_data   *data = NULL;
        if (cgltf_parse_file(&options, filename.c_str(), &data) != cgltf_result_success)
            return false;

        if (cgltf_load_buffers(&options, data, filename.c_str()) != cgltf_result_success)
            return false;

        auto root = data->scene;

        for (size_t i = 0; i < root->nodes_count; i++)
            processNode(resourceManager, model, data, root->nodes[i]);

        cgltf_free(data);
        return true;
    }

    static void processNode(ResourceManager *resourceManager, Model &model, cgltf_data *data, cgltf_node *gltfNode)
    {
        for (size_t i = 0; i < gltfNode->mesh->primitives_count; i++) {
            Mesh mesh = {};
            auto primitive = gltfNode->mesh->primitives[i];

            size_t             vertexSize = primitive.attributes[0].data->count;
            std::vector<float> temp(vertexSize * 4);

            // load vertices
            mesh.vertices.resize(vertexSize);
            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 3);

                for (size_t j = 0; j < vertexSize; j++) {
                    mesh.vertices[j].position.x = temp[j * 3 + 0];
                    mesh.vertices[j].position.y = temp[j * 3 + 1];
                    mesh.vertices[j].position.z = temp[j * 3 + 2];
                }
            }

            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 3);

                for (size_t j = 0; j < vertexSize; j++) {
                    mesh.vertices[j].normal.x = temp[j * 3 + 0];
                    mesh.vertices[j].normal.y = temp[j * 3 + 1];
                    mesh.vertices[j].normal.z = temp[j * 3 + 2];
                }
            }

            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 2);

                for (size_t j = 0; j < vertexSize; j++) {
                    mesh.vertices[j].uv_x = temp[j * 2 + 0];
                    mesh.vertices[j].uv_y = temp[j * 2 + 1];
                }
            }

            // load indices
            mesh.indices.resize(primitive.indices->count);
            cgltf_accessor_unpack_indices(primitive.indices, mesh.indices.data(), 4, mesh.indices.size());

            // load materials
            if (primitive.material) {
                Material material;

                if (primitive.material->has_pbr_metallic_roughness) {
                    if (primitive.material->pbr_metallic_roughness.base_color_texture.texture) {
                        const char *uri = primitive.material->pbr_metallic_roughness.base_color_texture.texture->image->uri;
                        material.baseColorTexture = resourceManager->loadTextureFromFile(resourceManager->getUniqueTextureName(), uri);
                    }
                }

                MaterialID materialID = resourceManager->addMaterial(resourceManager->getUniqueMaterialName(), material);
                mesh.materialID = materialID;
            }

            resourceManager->uploadMeshBuffers(mesh);

            MeshID meshID = resourceManager->addMesh(resourceManager->getUniqueMeshName(), mesh);
            model.meshes.push_back(meshID);
        }

        for (size_t i = 0; i < gltfNode->children_count; i++) {
            processNode(resourceManager, model, data, gltfNode->children[i]);
        }
    }
} // namespace gltf