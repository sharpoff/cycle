#include "core/gltf_loader.h"

#include "cgltf.h"
#include "graphics/material.h"
#include "graphics/mesh.h"

#include <limits>
#include <chrono>

#include "core/asset_manager.h"
#include "graphics/renderer.h"

namespace gltf
{
    static uint32_t uniqueNumber = 0;

    bool Loader::Load(Scene &scene, FilePath filename)
    {
        if (!std::filesystem::exists(filename))
            return false;

        cgltf_options options = {};
        cgltf_data *data = NULL;
        if (cgltf_parse_file(&options, filename.c_str(), &data) != cgltf_result_success)
            return false;

        if (cgltf_load_buffers(&options, data, filename.c_str()) != cgltf_result_success)
            return false;

        cgltf_scene *root = data->scene;
        if (!root) {
            cgltf_free(data);
            return false;
        }

        String baseDir = FilePath(filename).parent_path();

        for (size_t i = 0; i < root->nodes_count; i++) {
            ProcessNode(scene.nodes.emplace_back(), data, root->nodes[i], baseDir);
        }

        CalculateBounds(scene);

        cgltf_free(data);
        return true;
    }

    void Loader::ProcessNode(SceneNode &node, cgltf_data *data, cgltf_node *gltfNode, String baseDir)
    {
        if (gltfNode->mesh) {
            Mesh mesh = {};
            for (size_t i = 0; i < gltfNode->mesh->primitives_count; i++) {
                MeshPrimitive meshPrimitive = {};
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

                // load material
                if (primitive.material) {
                    Material *material = new Material();
                    cgltf_material *gltfMaterial = primitive.material;

                    if (gltfMaterial->has_pbr_metallic_roughness) {
                        // base color
                        if (gltfMaterial->pbr_metallic_roughness.base_color_texture.texture) {
                            cgltf_image *gltfImage = gltfMaterial->pbr_metallic_roughness.base_color_texture.texture->image;
                            String name = gltfImage->name ? gltfImage->name : GetUniqueName();

                            if (gltfImage->uri) {
                                material->baseColorTex = gAssetManager->CreateTexture(baseDir / FilePath(gltfImage->uri), name);
                            } else if (gltfImage->buffer_view && gltfImage->buffer_view->buffer) {
                                unsigned char *data = const_cast<unsigned char *>(cgltf_buffer_view_data(gltfImage->buffer_view));
                                material->baseColorTex = gAssetManager->CreateTexture(data, gltfImage->buffer_view->buffer->size, name);
                            }
                        }

                        // metallic roughness
                        if (gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture) {
                            cgltf_image *gltfImage = gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
                            String name = gltfImage->name ? gltfImage->name : GetUniqueName();

                            if (gltfImage->uri) {
                                material->metallicRoughnessTex = gAssetManager->CreateTexture(baseDir / FilePath(gltfImage->uri), name);
                            } else if (gltfImage->buffer_view && gltfImage->buffer_view->buffer) {
                                unsigned char *data = const_cast<unsigned char *>(cgltf_buffer_view_data(gltfImage->buffer_view));
                                material->metallicRoughnessTex = gAssetManager->CreateTexture(data, gltfImage->buffer_view->buffer->size, name);
                            }

                            material->metallicFactor = gltfMaterial->pbr_metallic_roughness.metallic_factor;
                            material->roughnessFactor = gltfMaterial->pbr_metallic_roughness.roughness_factor;
                        }
                    }

                    // normal
                    if (gltfMaterial->normal_texture.texture) {
                        cgltf_image *gltfImage = gltfMaterial->normal_texture.texture->image;
                        String name = gltfImage->name ? gltfImage->name : GetUniqueName();

                        if (gltfImage->uri) {
                            material->normalTex = gAssetManager->CreateTexture(baseDir / FilePath(gltfImage->uri), name);
                        } else if (gltfImage->buffer_view && gltfImage->buffer_view->buffer) {
                            unsigned char *data = const_cast<unsigned char *>(cgltf_buffer_view_data(gltfImage->buffer_view));
                            material->normalTex = gAssetManager->CreateTexture(data, gltfImage->buffer_view->buffer->size, name);
                        }
                    }

                    // emissive
                    if (gltfMaterial->emissive_texture.texture) {
                        cgltf_image *gltfImage = gltfMaterial->emissive_texture.texture->image;
                        String name = gltfImage->name ? gltfImage->name : GetUniqueName();

                        if (gltfImage->uri) {
                            material->emissiveTex = gAssetManager->CreateTexture(baseDir / FilePath(gltfImage->uri), name);
                        } else if (gltfImage->buffer_view && gltfImage->buffer_view->buffer) {
                            unsigned char *data = const_cast<unsigned char *>(cgltf_buffer_view_data(gltfImage->buffer_view));
                            material->emissiveTex = gAssetManager->CreateTexture(data, gltfImage->buffer_view->buffer->size, name);
                        }
                    }

                    meshPrimitive.gltfMaterialIndex = cgltf_material_index(data, gltfMaterial);
                    meshPrimitive.material = material;
                }

                meshPrimitive.vertices = vertices;
                meshPrimitive.indices = indices;
                gRenderer->GetDevice().UploadMeshGpuData(meshPrimitive.vertexBuffer, meshPrimitive.vertices, meshPrimitive.indexBuffer, meshPrimitive.indices);

                mesh.primitives.push_back(meshPrimitive);
            }

            node.mesh = mesh;
        }

        for (size_t i = 0; i < gltfNode->children_count; i++) {
            ProcessNode(node.children.emplace_back(), data, gltfNode->children[i], baseDir);
        }
    }

    String Loader::GetUniqueName()
    {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        return std::to_string(ms) + std::to_string(uniqueNumber++);
    }

    bool Loader::CalculateBounds(Scene &scene)
    {
        if (scene.nodes.empty())
            return false;

        vec3 min = vec3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        vec3 max = vec3(std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min());

        for (auto &node : scene.nodes) {
            for (auto &prim : node.mesh.primitives) {
                for (size_t i = 0; i < prim.indices.size(); i++) {
                    Vertex &vert = prim.vertices[prim.indices[i]];

                    min = glm::min(min, vert.position);
                    max = glm::max(max, vert.position);
                }
            }
        }

        scene.bounds.extents = vec3(max - min) * 0.5f;

        return true;
    }
} // namespace gltf