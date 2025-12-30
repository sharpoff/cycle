#include "cycle/gltf_loader.h"

#include <filesystem>

#include "cgltf.h"
#include "cycle/types/id.h"
#include "cycle/types/material.h"
#include "cycle/types/mesh.h"

#include "cycle/managers/mesh_manager.h"
#include "cycle/managers/texture_manager.h"
#include "cycle/managers/material_manager.h"

#include "cycle/globals.h"

namespace gltf
{
    static void processNode(Model &model, cgltf_data *data, cgltf_node *gltfNode, String baseDir);

    bool load(Model &model, const String &filename)
    {
        if (filename.empty())
            return false;

        cgltf_options options = {};
        cgltf_data   *data = NULL;
        if (cgltf_parse_file(&options, filename.c_str(), &data) != cgltf_result_success)
            return false;

        if (cgltf_load_buffers(&options, data, filename.c_str()) != cgltf_result_success)
            return false;

        cgltf_scene *root = data->scene;
        if (!root) {
            cgltf_free(data);
            return false;
        }

        String baseDir = std::filesystem::path(filename).parent_path();

        for (size_t i = 0; i < root->nodes_count; i++)
            processNode(model, data, root->nodes[i], baseDir);

        cgltf_free(data);
        return true;
    }

    static void processNode(Model &model, cgltf_data *data, cgltf_node *gltfNode, String baseDir)
    {
        for (size_t i = 0; i < gltfNode->mesh->primitives_count; i++) {
            Mesh mesh = {};
            auto primitive = gltfNode->mesh->primitives[i];

            size_t vertexSize = primitive.attributes[0].data->count;
            std::vector<float> temp(vertexSize * 4);

            // load vertices
            Vector<Vertex> vertices(vertexSize);
            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 3);

                for (size_t j = 0; j < vertexSize; j++) {
                    vertices[j].position.x = temp[j * 3 + 0];
                    vertices[j].position.y = temp[j * 3 + 1];
                    vertices[j].position.z = temp[j * 3 + 2];
                }
            }

            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 3);

                for (size_t j = 0; j < vertexSize; j++) {
                    vertices[j].normal.x = temp[j * 3 + 0];
                    vertices[j].normal.y = temp[j * 3 + 1];
                    vertices[j].normal.z = temp[j * 3 + 2];
                }
            }

            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 2);

                for (size_t j = 0; j < vertexSize; j++) {
                    vertices[j].uv_x = temp[j * 2 + 0];
                    vertices[j].uv_y = temp[j * 2 + 1];
                }
            }

            // load indices
            Vector<uint32_t> indices(primitive.indices->count);
            cgltf_accessor_unpack_indices(primitive.indices, indices.data(), 4, indices.size());

            // load materials
            if (primitive.material) {
                Material material = {};
                cgltf_material *gltfMaterial = primitive.material;

                if (gltfMaterial->has_pbr_metallic_roughness) {
                    // base color
                    if (gltfMaterial->pbr_metallic_roughness.base_color_texture.texture) {
                        cgltf_image *gltfImage = gltfMaterial->pbr_metallic_roughness.base_color_texture.texture->image;
                        const char *uri = gltfImage->uri;

                        if (uri) {
                            material.baseColorTexID = g_textureManager->createTexture(baseDir / std::filesystem::path(uri));
                        }
                    }

                    // metallic roughness
                    if (gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture) {
                        cgltf_image *gltfImage = gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
                        const char *uri = gltfImage->uri;

                        if (uri) {
                            material.metallicRoughnessTexID = g_textureManager->createTexture(baseDir / std::filesystem::path(uri));
                        }
                    }

                    // normal
                    if (gltfMaterial->normal_texture.texture) {
                        cgltf_image *gltfImage = gltfMaterial->normal_texture.texture->image;
                        const char *uri = gltfImage->uri;

                        if (uri) {
                            material.normalTexID = g_textureManager->createTexture(baseDir / std::filesystem::path(uri));
                        }
                    }

                    // emissive
                    if (gltfMaterial->emissive_texture.texture) {
                        cgltf_image *gltfImage = gltfMaterial->emissive_texture.texture->image;
                        const char *uri = gltfImage->uri;

                        if (uri) {
                            material.emissiveTexID = g_textureManager->createTexture(baseDir / std::filesystem::path(uri));
                        }
                    }
                }

                mesh.materialID = g_materialManager->addMaterial(material);
            }

            MeshID meshID = g_meshManager->addMesh(mesh);
            model.meshes.push_back(meshID);

            g_meshManager->uploadMeshData(meshID, vertices, indices);
        }

        for (size_t i = 0; i < gltfNode->children_count; i++) {
            processNode(model, data, gltfNode->children[i], baseDir);
        }
    }
} // namespace gltf