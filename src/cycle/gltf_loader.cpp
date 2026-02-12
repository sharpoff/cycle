#include "cycle/gltf_loader.h"

#include "cgltf.h"
#include "cycle/types/material.h"
#include "cycle/types/mesh.h"

#include "cycle/managers/model_manager.h"
#include "cycle/managers/texture_manager.h"
#include "cycle/managers/material_manager.h"

namespace gltf
{
    static void processNode(Model &model, cgltf_data *data, cgltf_node *gltfNode, String baseDir);

    bool load(Model &model, const std::filesystem::path &filepath)
    {
        if (filepath.empty() || !std::filesystem::exists(filepath))
            return false;

        cgltf_options options = {};
        cgltf_data   *data = NULL;
        if (cgltf_parse_file(&options, filepath.c_str(), &data) != cgltf_result_success)
            return false;

        if (cgltf_load_buffers(&options, data, filepath.c_str()) != cgltf_result_success)
            return false;

        cgltf_scene *root = data->scene;
        if (!root) {
            cgltf_free(data);
            return false;
        }

        String baseDir = std::filesystem::path(filepath).parent_path();

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
            
            // position
            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_position, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 3);

                for (size_t j = 0; j < vertexSize; j++) {
                    vertices[j].position[0] = temp[j * 3 + 0];
                    vertices[j].position[1] = temp[j * 3 + 1];
                    vertices[j].position[2] = temp[j * 3 + 2];
                }
            }

            // normal
            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_normal, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 3);

                for (size_t j = 0; j < vertexSize; j++) {
                    vertices[j].normal[0] = temp[j * 3 + 0];
                    vertices[j].normal[1] = temp[j * 3 + 1];
                    vertices[j].normal[2] = temp[j * 3 + 2];
                }
            }

            // uv
            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_texcoord, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 2);

                for (size_t j = 0; j < vertexSize; j++) {
                    vertices[j].uv_x = temp[j * 2 + 0];
                    vertices[j].uv_y = temp[j * 2 + 1];
                }
            }

            // tangent
            if (auto accessor = cgltf_find_accessor(&primitive, cgltf_attribute_type_tangent, 0); accessor) {
                cgltf_accessor_unpack_floats(accessor, &temp[0], vertexSize * 4);

                for (size_t j = 0; j < vertexSize; j++) {
                    vertices[j].tangent[0] = temp[j * 4 + 0];
                    vertices[j].tangent[1] = temp[j * 4 + 1];
                    vertices[j].tangent[2] = temp[j * 4 + 2];
                    vertices[j].tangent[3] = temp[j * 4 + 3];
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

                        if (gltfImage->uri) {
                            material.baseColorTexID = TextureManager::get()->createTexture(baseDir / std::filesystem::path(gltfImage->uri));
                        } else if (gltfImage->buffer_view && gltfImage->buffer_view->buffer) {
                            unsigned char *data = const_cast<unsigned char *>(cgltf_buffer_view_data(gltfImage->buffer_view));
                            material.baseColorTexID = TextureManager::get()->createTextureFromMem(data, gltfImage->buffer_view->buffer->size);
                        }
                    }

                    // metallic roughness
                    if (gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture) {
                        cgltf_image *gltfImage = gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture->image;

                        if (gltfImage->uri) {
                            material.metallicRoughnessTexID = TextureManager::get()->createTexture(baseDir / std::filesystem::path(gltfImage->uri));
                        } else if (gltfImage->buffer_view && gltfImage->buffer_view->buffer) {
                            unsigned char *data = const_cast<unsigned char *>(cgltf_buffer_view_data(gltfImage->buffer_view));
                            material.metallicRoughnessTexID = TextureManager::get()->createTextureFromMem(data, gltfImage->buffer_view->buffer->size);
                        }
    
                        material.metallicFactor = gltfMaterial->pbr_metallic_roughness.metallic_factor;
                        material.roughnessFactor = gltfMaterial->pbr_metallic_roughness.roughness_factor;
                    }
                }

                // normal
                if (gltfMaterial->normal_texture.texture) {
                    cgltf_image *gltfImage = gltfMaterial->normal_texture.texture->image;

                    if (gltfImage->uri) {
                        material.normalTexID = TextureManager::get()->createTexture(baseDir / std::filesystem::path(gltfImage->uri), VK_FORMAT_R8G8B8A8_UNORM);
                    } else if (gltfImage->buffer_view && gltfImage->buffer_view->buffer) {
                        unsigned char *data = const_cast<unsigned char *>(cgltf_buffer_view_data(gltfImage->buffer_view));
                        material.normalTexID = TextureManager::get()->createTextureFromMem(data, gltfImage->buffer_view->buffer->size, VK_FORMAT_R8G8B8A8_UNORM);
                    }
                }

                // emissive
                if (gltfMaterial->emissive_texture.texture) {
                    cgltf_image *gltfImage = gltfMaterial->emissive_texture.texture->image;
                    const char *uri = gltfImage->uri;

                    if (uri) {
                        material.emissiveTexID = TextureManager::get()->createTexture(baseDir / std::filesystem::path(uri));
                    } else if (gltfImage->buffer_view && gltfImage->buffer_view->buffer) {
                        unsigned char *data = const_cast<unsigned char *>(cgltf_buffer_view_data(gltfImage->buffer_view));
                        material.emissiveTexID = TextureManager::get()->createTextureFromMem(data, gltfImage->buffer_view->buffer->size);
                    }

                    // material.emissiveFactor = glm::make_vec3(gltfMaterial->emissive_factor);
                }

                mesh.materialID = MaterialManager::get()->addMaterial(material);
            }

            ModelManager::get()->uploadMeshData(mesh, vertices, indices);
            model.meshes.push_back(mesh);
        }

        for (size_t i = 0; i < gltfNode->children_count; i++) {
            processNode(model, data, gltfNode->children[i], baseDir);
        }
    }
} // namespace gltf