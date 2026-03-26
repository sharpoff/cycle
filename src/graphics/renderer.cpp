#include "renderer.h"

#include <filesystem>

#include "core/asset_manager.h"
#include "core/filesystem.h"
#include "game/world.h"
#include "graphics/material.h"
#include "graphics/push_constants.h"
#include "graphics/scene_info.h"
#include "graphics/vulkan_helpers.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

void Renderer::Init(SDL_Window *window)
{
    device.Init(window);
    CreateAttachmentImages();

    { // shadowmap image
        TextureCreateInfo createInfo = {
            .width = SHADOWMAP_DIM,
            .height = SHADOWMAP_DIM,
            .mipLevels = 1,
            .sampleCount = device.maxSampleCount,
            .type = VK_IMAGE_VIEW_TYPE_2D,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .format = VK_FORMAT_D32_SFLOAT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        };

        for (uint32_t i = 0; i < SHADOWMAP_CASCADES; i++) {
            shadowmapImages[i] = device.CreateTexture(createInfo);
        }
    }

    // create global data buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(SceneInfo),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
            sceneInfoBuffers[i] = device.CreateBuffer(createInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }

    // create cascades matrices buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(Cascade) * cascades.size(),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };

        cascadesBuffer = device.CreateBuffer(createInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    // create common samplers
    { // linear
        SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.maxLod = 4;

        linearSampler = device.CreateSampler(samplerCreateInfo);
    }

    { // nearest
        SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.maxLod = 4;

        nearestSampler = device.CreateSampler(samplerCreateInfo);
    }

    CreatePipelines();

    // write static descriptors
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        device.WriteDescriptor(0, sceneInfoBuffers[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
    device.WriteDescriptor(2, linearSampler, VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_LINEAR_ID);
    device.WriteDescriptor(2, nearestSampler, VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_NEAREST_ID);

    device.UpdateDescriptors();
}

void Renderer::Shutdown()
{
    device.WaitIdle();

    DestroyAttachmentImages();

    for (auto &shadowmap : shadowmapImages) {
        device.DestroyTexture(shadowmap);
    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        device.DestroyBuffer(sceneInfoBuffers[i]);
    }
    device.DestroyBuffer(materialsBuffer);
    device.DestroyBuffer(lightsBuffer);
    device.DestroyBuffer(cascadesBuffer);

    device.DestroySampler(linearSampler);
    device.DestroySampler(nearestSampler);

    DestroyPipelines();

    device.Shutdown();
    delete gRenderer;
}

void Renderer::AddTextureToDescriptor(Texture *texture)
{
    if (texture) {
        texture->SetID(descriptorTextures.size());
        descriptorTextures.push_back(texture);
    }
}

void Renderer::AddMaterialToDescriptor(Material *material)
{
    if (material) {
        material->SetID(descriptorMaterials.size());
        descriptorMaterials.push_back(material);
    }
}

void Renderer::LoadDynamicResources()
{
    // create materials buffer
    if (!descriptorMaterials.empty()) {
        // if (materialsBuffer->size > 0) { // delete existing materials buffer
        //     device.DestroyBuffer(materialsBuffer);
        // }

        Func<uint32_t(Texture*)> getIdOrNull = [](Texture *tex) {
            if (tex)
                return tex->GetID();
            return UINT32_MAX;
        };

        Vector<GPUMaterial> gpuMaterials(descriptorMaterials.size());
        for (uint32_t i = 0; i < descriptorMaterials.size(); i++) {
            gpuMaterials[i].baseColorTexID = getIdOrNull(descriptorMaterials[i]->baseColorTex);
            gpuMaterials[i].metallicRoughnessTexID = getIdOrNull(descriptorMaterials[i]->metallicRoughnessTex);
            gpuMaterials[i].normalTexID = getIdOrNull(descriptorMaterials[i]->normalTex);
            gpuMaterials[i].emissiveTexID = getIdOrNull(descriptorMaterials[i]->emissiveTex);
            gpuMaterials[i].roughnessFactor = descriptorMaterials[i]->roughnessFactor;
            gpuMaterials[i].metallicFactor = descriptorMaterials[i]->metallicFactor;
        }

        const BufferCreateInfo createInfo = {
            .size = sizeof(GPUMaterial) * gpuMaterials.size(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        materialsBuffer = device.CreateBuffer(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        device.UploadBufferData(materialsBuffer, gpuMaterials.data(), createInfo.size);
    }

    // create lights buffer
    // lightEntities = EntityManager::get()->lights.getEntities();
    // if (lightEntities.size() > 0) {
    //     if (lightsBuffer.size > 0) { // delete existing lights buffer
    //         device.destroyBuffer(lightsBuffer);
    //     }

    //     updateGPULights();

    //     const BufferCreateInfo createInfo = {
    //         .size = sizeof(GPULight) * gpuLights.size(),
    //         .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    //     };

    //     lightsBuffer = device.createBuffer(createInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
    //     device.uploadBufferData(lightsBuffer, gpuLights.data(), createInfo.size);
    // }

    // write dynamic descriptors
    for (size_t i = 0; i < descriptorTextures.size(); i++)
        device.WriteDescriptor(1, descriptorTextures[i], VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorTextures[i]->GetID());

    if (!descriptorMaterials.empty())
        device.WriteDescriptor(3, materialsBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // if (lightEntities.size() > 0)
    //     device.writeDescriptor(4, lightsBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    device.WriteDescriptor(5, cascadesBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    device.UpdateDescriptors();
}

void Renderer::ReloadShaders()
{
    device.WaitIdle();

    CompileShaders();

    DestroyPipelines();
    CreatePipelines();
}

void Renderer::DrawFrame()
{
    assert(camera_ && "Camera should be set!");

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (cmd = device.BeginCommandBuffer(); cmd == VK_NULL_HANDLE) {
        // resize swapchain and recreate attachment images
        ResizeWindow();
        return;
    }

    UpdateDynamicData();

    // pre-render barriers
    barriers.TransitionImage2(colorImage->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.TransitionImage2(depthImage->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    for (auto &shadowmap : shadowmapImages)
        barriers.TransitionImage2(shadowmap->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    barriers.TransitionImage2(device.GetSwapchainImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.FlushBarriers(cmd);

    const uint32_t currentFrame = device.GetCurrentFrame();
    VkExtent2D renderArea = {device.GetSwapchainWidth(), device.GetSwapchainHeight()};
    const float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    VkRenderingAttachmentInfo depthAttachment = vulkan::CreateAttachmentInfo(depthImage->view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false, true);

    //===========================
    // Render skybox
    //===========================
#if 1
    {
        vulkan::BeginDebugLabel(cmd, "skybox");
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::CreateAttachmentInfo(colorImage->view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false, true),
        };
        VkRenderingInfo renderingInfo = vulkan::CreateRenderingInfo(renderArea, colorAttachments, &depthAttachment);
        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport = {};
        viewport.width = renderArea.width;
        viewport.height = renderArea.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = renderArea.width;
        scissor.extent.height = renderArea.height;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline->layout, 0, 1, &device.GetBindlessDescriptor(), 0, 0);

        // draw skybox cube
        DrawModel(cmd, gAssetManager->GetModel("cube"), mat4(1.0f));

        vkCmdEndRendering(cmd);
        vulkan::EndDebugLabel(cmd);
    }
#endif

    //===========================
    // Render shadowmap
    //===========================
#if 0
    vulkan::BeginDebugLabel(cmd, "shadowmapping");
    for (uint32_t i = 0; i < shadowmapImages.size(); i++) {
        Texture * shadowmap = shadowmapImages[i];

        VkRenderingAttachmentInfo shadowmapDepthAttachment = vulkan::CreateAttachmentInfo(shadowmap->view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false, true);

        VkRenderingInfo renderingInfo = vulkan::CreateRenderingInfo({shadowmap->width, shadowmap->height}, {}, &shadowmapDepthAttachment);
        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport = {};
        viewport.width = shadowmap->width;
        viewport.height = shadowmap->height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0.0f;
        scissor.offset.y = 0.0f;
        scissor.extent.width = renderArea.width;
        scissor.extent.height = renderArea.height;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmapPipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmapPipeline->layout, 0, 1, &device.GetBindlessDescriptor(), 0, 0);

        // render all entities that cast shadows
        for (Entity *object : gWorld->GetObjects()) {
            if (!object || (object->GetDrawFlags() & Entity::kCastShadows) != Entity::kCastShadows)
                continue;

            DrawModel(cmd, object->GetModel(), object->GetWorldMatrix());
        }

        vkCmdEndRendering(cmd);
    }
    vulkan::EndDebugLabel(cmd);
#endif

    //===========================
    // Render models
    //===========================
#if 1
    {
        vulkan::BeginDebugLabel(cmd, "models");
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::CreateAttachmentInfo(colorImage->view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true, true),
        };

        VkRenderingInfo renderingInfo = vulkan::CreateRenderingInfo(renderArea, colorAttachments, &depthAttachment);
        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport = {};
        viewport.width = renderArea.width;
        viewport.height = renderArea.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = renderArea.width;
        scissor.extent.height = renderArea.height;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline->layout, 0, 1, &device.GetBindlessDescriptor(), 0, 0);

        // render all entities that are visible
        for (Entity *entity : gWorld->GetEntities()) {
            if (!entity || (entity->GetDrawFlags() & Entity::kVisible) != Entity::kVisible)
                continue;

            Model *model = entity->GetModel();
            DrawModel(cmd, model, entity->GetWorldMatrix());
        }

        vkCmdEndRendering(cmd);
        vulkan::EndDebugLabel(cmd);
    }
#endif

    //===========================
    // Render imgui
    //===========================
#if 1
    {
        vulkan::BeginDebugLabel(cmd, "imgui");
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::CreateAttachmentInfo(colorImage->view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true, true, device.GetSwapchainImageView(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
        };

        VkRenderingInfo renderingInfo = vulkan::CreateRenderingInfo(renderArea, colorAttachments, &depthAttachment);
        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport = {};
        viewport.width = renderArea.width;
        viewport.height = renderArea.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.extent.width = renderArea.width;
        scissor.extent.height = renderArea.height;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // draw
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();
        DrawImGuiDebug();

        ImGui::Render();

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        vkCmdEndRendering(cmd);
        vulkan::EndDebugLabel(cmd);
    }
#endif

    // post-render barriers
    barriers.TransitionImage2(device.GetSwapchainImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.FlushBarriers(cmd);

    device.EndCommandBuffer(cmd);

    if (!device.SwapchainPresent()) {
        ResizeWindow();
        return;
    }
}

void Renderer::DrawImGuiDebug()
{
    ImGui::Begin("Entities");
    for (auto &entity : gWorld->GetEntities()) {
        ImGui::Selectable(entity->GetName().c_str());
    }
    ImGui::End();
}

vec2 Renderer::GetScreenSize()
{
    return vec2(device.GetSwapchainWidth(), device.GetSwapchainHeight());
}

float Renderer::GetAspectRatio()
{
    vec2 screenSize = GetScreenSize();
    return float(screenSize.x) / screenSize.y;
}

void Renderer::DrawModel(VkCommandBuffer cmd, Model *model, mat4 worldMatrix)
{
    if (!model)
        return;

    for (auto &mesh : model->meshes) {
        DrawMesh(cmd, mesh, worldMatrix);
    }
}

void Renderer::DrawMesh(VkCommandBuffer cmd, Mesh &mesh, mat4 worldMatrix)
{
    // draw all meshes of a model
    for (MeshPrimitive &prim : mesh.primitives) {
        MeshPushConstants push = {};
        push.worldMatrix = worldMatrix * prim.worldMatrix;
        push.vertexBufferAddress = prim.vertexBuffer->address;
        push.materialId = prim.material ? prim.material->GetID() : UINT32_MAX;
        vkCmdPushConstants(cmd, meshPipeline->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);

        vkCmdBindIndexBuffer(cmd, prim.indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, prim.indices.size(), 1, 0, 0, 0);
    }
}

void Renderer::ResizeWindow()
{
    // recreate all swapchain dependant resources
    DestroyAttachmentImages();
    CreateAttachmentImages();
}

void Renderer::CreateAttachmentImages()
{
    { // color image
        TextureCreateInfo createInfo = {
            .width = device.GetSwapchainWidth(),
            .height = device.GetSwapchainHeight(),
            .mipLevels = 1,
            .sampleCount = device.maxSampleCount,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .format = device.GetSurfaceFormat().format,
            .debugName = "color"};

        colorImage = device.CreateTexture(createInfo);
    }

    { // depth image
        TextureCreateInfo createInfo = {
            .width = device.GetSwapchainWidth(),
            .height = device.GetSwapchainHeight(),
            .mipLevels = 1,
            .sampleCount = device.maxSampleCount,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .format = VK_FORMAT_D32_SFLOAT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .debugName = "depth"};

        depthImage = device.CreateTexture(createInfo);
    }
}

void Renderer::DestroyAttachmentImages()
{
    device.WaitIdle();
    device.DestroyTexture(colorImage);
    device.DestroyTexture(depthImage);
}

void Renderer::CompileShaders()
{
    std::filesystem::create_directory(shadersBinaryDir);
    for (auto &entry : std::filesystem::directory_iterator(shadersDir)) {
        if (!entry.is_regular_file())
            continue;

        FilePath filepath = entry.path();
        String extension = filepath.extension();
        if (extension == ".vert" || extension == ".frag" || extension == ".comp" || extension == ".tesc" || extension == ".tese") {
            String filename = filepath.filename();

            // HACK: this uses command line to recompile all shaders
            String cmd = "glslangValidator -V " + filepath.string() + " -o " + String(shadersBinaryDir / (filename + ".spv"));
            system(cmd.c_str());
        }
    }
}

void Renderer::CreatePipelines()
{
    { // mesh pipeline
        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants)}};

        RenderPipelineCreateInfo createInfo = {
            .pushConstantRanges = pushConstantRanges,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .sampleCount = device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB},
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = ReadFile(shadersBinaryDir / "mesh.vert.spv", true),
            .fragmentCode = ReadFile(shadersBinaryDir / "mesh.frag.spv", true),
        };

        meshPipeline = device.CreateRenderPipeline(createInfo);
    }

    { // skybox pipeline
        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants)}};

        RenderPipelineCreateInfo createInfo = {
            .pushConstantRanges = pushConstantRanges,
            .cullMode = VK_CULL_MODE_FRONT_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .sampleCount = device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB},
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = ReadFile(shadersBinaryDir / "skybox.vert.spv", true),
            .fragmentCode = ReadFile(shadersBinaryDir / "skybox.frag.spv", true),
        };

        skyboxPipeline = device.CreateRenderPipeline(createInfo);
    }

    { // shadowmap pipeline
        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DepthPushConstants)}};

        RenderPipelineCreateInfo createInfo = {
            .pushConstantRanges = pushConstantRanges,
            .cullMode = VK_CULL_MODE_NONE,
            .sampleCount = device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = ReadFile(shadersBinaryDir / "shadowmap.vert.spv", true),
        };

        shadowmapPipeline = device.CreateRenderPipeline(createInfo);
    }
}

void Renderer::DestroyPipelines()
{
    device.DestroyRenderPipeline(meshPipeline);
    device.DestroyRenderPipeline(skyboxPipeline);
    device.DestroyRenderPipeline(shadowmapPipeline);
}

void Renderer::UpdateDynamicData()
{
    // lights
    UpdateGpuLights();
    if (gpuLights.size() > 0) {
        device.UploadBufferData(lightsBuffer, gpuLights.data(), sizeof(GPULight) * gpuLights.size());

        // HACK: uses first light
        UpdateShadowmapCascades(*camera_, gpuLights[0].direction);
        device.UploadBufferData(cascadesBuffer, cascades.data(), cascades.size() * sizeof(Cascade));
    }

    // scene info
    SceneInfo sceneInfo = {};
    sceneInfo.view = camera_->GetView();
    sceneInfo.projection = camera_->GetProjection();
    sceneInfo.cameraPos = camera_->GetPosition();
    sceneInfo.lightsCount = gpuLights.size();
    sceneInfo.skyboxTexID = gAssetManager->GetTexture("skybox")->GetID();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        device.UploadBufferData(sceneInfoBuffers[i], &sceneInfo, sizeof(sceneInfo));
}

void Renderer::UpdateGpuLights()
{
    gpuLights.clear();
    // TODO: check if entity is dirty (changed position, direction, color, etc.), so we don't update every light
    // for (EntityID lightID : lightEntities) {
    //     LightComponent     *lightComponent = EntityManager::get()->lights.getComponent(lightID);
    //     TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(lightID);
    //     if (!lightComponent || !transformComponent)
    //         continue;

    //     GPULight &light = gpuLights.emplace_back();
    //     light.intensity = 1.0f;
    //     light.position = math::getPosition(transformComponent->transform);
    //     light.direction = lightComponent->direction;
    //     light.color = lightComponent->color;
    //     light.type = lightComponent->lightType;
    // }
}

// references: https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
//          and https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp
void Renderer::UpdateShadowmapCascades(Camera &camera, vec3 lightDir)
{
    float cascadeSplits[SHADOWMAP_CASCADES];

    float nearClip = camera.GetNearClip();
    float farClip = camera.GetFarClip();
    float clipRange = farClip - nearClip;

    float minZ = nearClip;
    float maxZ = nearClip + clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    // Calculate split depths based on view camera frustum
    // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
    for (uint32_t i = 0; i < SHADOWMAP_CASCADES; i++) {
        float p = (i + 1) / static_cast<float>(SHADOWMAP_CASCADES);
        float log = minZ * std::pow(ratio, p);
        float uniform = minZ + range * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearClip) / clipRange;
    }

    // Calculate orthographic projection matrix for each cascade
    float lastSplitDist = 0.0;
    for (uint32_t i = 0; i < SHADOWMAP_CASCADES; i++) {
        float splitDist = cascadeSplits[i];

        glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, 1.0f, 0.0f),
            glm::vec3(1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f, -1.0f, 0.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
        };

        // Project frustum corners into world space
        glm::mat4 invCam = glm::inverse(camera.GetProjection() * camera.GetView());

        for (uint32_t j = 0; j < 8; j++) {
            glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
            frustumCorners[j] = invCorner / invCorner.w;
        }

        for (uint32_t j = 0; j < 4; j++) {
            glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
            frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
            frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
        }

        // Get frustum center
        glm::vec3 frustumCenter = glm::vec3(0.0f);
        for (uint32_t j = 0; j < 8; j++) {
            frustumCenter += frustumCorners[j];
        }
        frustumCenter /= 8.0f;

        float radius = 0.0f;
        for (uint32_t j = 0; j < 8; j++) {
            float distance = glm::length(frustumCorners[j] - frustumCenter);
            radius = glm::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3(radius);
        glm::vec3 minExtents = -maxExtents;

        glm::vec3 lightDir = glm::normalize(vec3(-0.1f, -0.5f, 0.0f));
        vec3 eye = frustumCenter - lightDir * -minExtents.z;
        glm::mat4 lightViewMatrix = glm::lookAt(eye, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

        // Store split distance and matrix in cascade
        cascades[i].depth = (camera.GetNearClip() + splitDist * clipRange) * -1.0f;
        cascades[i].viewProjection = lightOrthoMatrix * lightViewMatrix;

        lastSplitDist = cascadeSplits[i];
    }
}