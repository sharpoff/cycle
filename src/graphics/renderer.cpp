#include "graphics/renderer.h"

#include <filesystem>

#include "core/asset_manager.h"
#include "core/filesystem.h"
#include "core/logger.h"
#include "game/world.h"
#include "graphics/material.h"
#include "graphics/push_constants.h"
#include "graphics/scene_info.h"
#include "graphics/vulkan_helpers.h"
#include "graphics/barrier_merger.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

void Renderer::Initialize(Window *window)
{
    m_device.Initialize(window);
    CreateAttachmentImages();

    { // shadowmap image
        TextureCreateInfo createInfo = {
            .width = SHADOWMAP_DIM,
            .height = SHADOWMAP_DIM,
            .mipLevels = 1,
            .sampleCount = m_device.maxSampleCount,
            .type = VK_IMAGE_VIEW_TYPE_2D,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .format = VK_FORMAT_D32_SFLOAT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        };

        for (uint32_t i = 0; i < SHADOWMAP_CASCADES; i++) {
            m_device.CreateTexture(m_shadowmapImages[i], createInfo);
        }
    }

    // create global data buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(SceneInfo),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
            m_device.CreateBuffer(m_sceneInfoBuffers[i], createInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }

    // create cascades matrices buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(Cascade) * m_cascades.size(),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };

        m_device.CreateBuffer(m_cascadesBuffer, createInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
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

        m_device.CreateSampler(m_linearSampler, samplerCreateInfo);
    }

    { // nearest
        SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerCreateInfo.maxLod = 4;

        m_device.CreateSampler(m_nearestSampler, samplerCreateInfo);
    }

    CreatePipelines();

    // write static descriptors
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        m_device.WriteDescriptor(0, m_sceneInfoBuffers[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
    m_device.WriteDescriptor(2, m_linearSampler, VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_LINEAR_ID);
    m_device.WriteDescriptor(2, m_nearestSampler, VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_NEAREST_ID);

    m_device.UpdateDescriptors();
}

void Renderer::Shutdown()
{
    m_device.WaitIdle();

    DestroyAttachmentImages();

    for (auto &shadowmap : m_shadowmapImages) {
        m_device.DestroyTexture(shadowmap);
    }

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        m_device.DestroyBuffer(m_sceneInfoBuffers[i]);
    }
    m_device.DestroyBuffer(m_materialsBuffer);
    m_device.DestroyBuffer(m_lightsBuffer);
    m_device.DestroyBuffer(m_cascadesBuffer);

    m_device.DestroySampler(m_linearSampler);
    m_device.DestroySampler(m_nearestSampler);

    DestroyPipelines();

    m_device.Shutdown();

    if (gRenderer)
        delete gRenderer;
}

void Renderer::AddTextureToDescriptor(uint32_t textureIndex)
{
    m_descriptorTextureIndices.push_back(textureIndex);
}

void Renderer::AddMaterialToDescriptor(uint32_t materialIndex)
{
    m_descriptorMaterialIndices.push_back(materialIndex);
}

void Renderer::LoadResources()
{
    // create materials buffer
    if (!m_descriptorMaterialIndices.empty()) {
        // if (materialsBuffer->size > 0) { // delete existing materials buffer
        //     device.DestroyBuffer(materialsBuffer);
        // }

        Vector<GPUMaterial> gpuMaterials(m_descriptorMaterialIndices.size());
        for (uint32_t i = 0; i < m_descriptorMaterialIndices.size(); i++) {
            Material *material = gAssetManager->GetMaterialByIndex(m_descriptorMaterialIndices[i]);
            gpuMaterials[i].baseColorTexID = material->baseColorTextureIndex;
            gpuMaterials[i].metallicRoughnessTexID = material->metallicRoughnessTextureIndex;
            gpuMaterials[i].normalTexID = material->normalTextureIndex;
            gpuMaterials[i].emissiveTexID = material->emissiveTextureIndex;
            gpuMaterials[i].roughnessFactor = material->roughnessFactor;
            gpuMaterials[i].metallicFactor = material->metallicFactor;
        }

        const BufferCreateInfo createInfo = {
            .size = sizeof(GPUMaterial) * gpuMaterials.size(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        m_device.CreateBuffer(m_materialsBuffer, createInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        m_device.UploadBufferData(m_materialsBuffer, gpuMaterials.data(), createInfo.size);
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
    for (size_t i = 0; i < m_descriptorTextureIndices.size(); i++)
        m_device.WriteDescriptor(1, *gAssetManager->GetTextureByIndex(m_descriptorTextureIndices[i]), VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, m_descriptorTextureIndices[i]);

    if (!m_descriptorMaterialIndices.empty())
        m_device.WriteDescriptor(3, m_materialsBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    // if (lightEntities.size() > 0)
    //     device.writeDescriptor(4, lightsBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    m_device.WriteDescriptor(5, m_cascadesBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    m_device.UpdateDescriptors();
}

void Renderer::ReloadShaders()
{
    m_device.WaitIdle();

    CompileShaders();

    DestroyPipelines();
    CreatePipelines();
}

void Renderer::RenderFrame()
{
    if (firstRun) {
        LoadResources();
        firstRun = false;
    }

    assert(m_camera && "Camera should be set!");

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (cmd = m_device.BeginCommandBuffer(); cmd == VK_NULL_HANDLE) {
        // resize swapchain and recreate attachment images
        ResizeWindow();
        return;
    }

    UpdateDynamicData();

    // pre-render barriers
    BarrierMerger barriers;
    barriers.TransitionImage2(m_colorImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.TransitionImage2(m_depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    for (auto &shadowmap : m_shadowmapImages)
        barriers.TransitionImage2(shadowmap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    barriers.TransitionImage2(m_device.GetSwapchainImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.FlushBarriers(cmd);

    const uint32_t currentFrame = m_device.GetCurrentFrameIndex();
    VkExtent2D renderArea = {m_device.GetSwapchainWidth(), m_device.GetSwapchainHeight()};
    const float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    VkRenderingAttachmentInfo depthAttachment = vulkan::CreateAttachmentInfo(m_depthImage.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false, true);

    //===========================
    // Render skybox
    //===========================
#if 0
    {
        vulkan::BeginDebugLabel(cmd, "skybox");
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::CreateAttachmentInfo(m_colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false, true),
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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxPipeline->pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxPipeline->layout, 0, 1, &m_device.GetBindlessDescriptor(), 0, 0);

        // draw skybox cube
        DrawModel(cmd, gAssetManager->GetModelByName("cube"), mat4(1.0f));

        vkCmdEndRendering(cmd);
        vulkan::EndDebugLabel(cmd);
    }
#endif

    //===========================
    // Render shadowmap
    //===========================
#if 0
    vulkan::BeginDebugLabel(cmd, "shadowmapping");
    for (uint32_t i = 0; i < m_shadowmapImages.size(); i++) {
        Texture &shadowmap = m_shadowmapImages[i];

        VkRenderingAttachmentInfo shadowmapDepthAttachment = vulkan::CreateAttachmentInfo(shadowmap.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false, true);

        VkRenderingInfo renderingInfo = vulkan::CreateRenderingInfo({shadowmap.width, shadowmap.height}, {}, &shadowmapDepthAttachment);
        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport = {};
        viewport.width = shadowmap.width;
        viewport.height = shadowmap.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor = {};
        scissor.offset.x = 0.0f;
        scissor.offset.y = 0.0f;
        scissor.extent.width = renderArea.width;
        scissor.extent.height = renderArea.height;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowmapPipeline.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowmapPipeline.layout, 0, 1, &m_device.GetBindlessDescriptor(), 0, 0);

        // render all entities that cast shadows
        for (Entity *entity : gWorld->GetEntities()) {
            if (!entity || (entity->GetDrawFlags() & Entity::kCastShadows) != Entity::kCastShadows)
                continue;

            DrawModel(cmd, entity->GetModel(), entity->GetWorldMatrix());
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
            vulkan::CreateAttachmentInfo(m_colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true, true),
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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipeline.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshPipeline.layout, 0, 1, &m_device.GetBindlessDescriptor(), 0, 0);

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
            vulkan::CreateAttachmentInfo(m_colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true, true, m_device.GetSwapchainImageView(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
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
    barriers.TransitionImage2(m_device.GetSwapchainImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.FlushBarriers(cmd);

    m_device.EndCommandBuffer(cmd);

    if (!m_device.SwapchainPresent()) {
        ResizeWindow();
        return;
    }
}

void Renderer::DrawImGuiDebug()
{
    ImGui::Begin("World entities");
    for (auto &entity : gWorld->GetEntities()) {
        ImGui::Selectable(entity->GetName().c_str());
    }
    ImGui::End();
}

vec2 Renderer::GetScreenSize()
{
    return vec2(m_device.GetSwapchainWidth(), m_device.GetSwapchainHeight());
}

float Renderer::GetAspectRatio()
{
    vec2 screenSize = GetScreenSize();
    return float(screenSize.x) / screenSize.y;
}

void Renderer::DrawModel(VkCommandBuffer cmd, Model *model, mat4 worldMatrix)
{
    if (!model) {
        LOGE("model is null!");
        return;
    }

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
        push.vertexBufferAddress = prim.vertexBuffer.address;
        push.materialId = prim.material ? prim.material->GetID() : UINT32_MAX;
        vkCmdPushConstants(cmd, m_meshPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);

        vkCmdBindIndexBuffer(cmd, prim.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
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
            .width = m_device.GetSwapchainWidth(),
            .height = m_device.GetSwapchainHeight(),
            .mipLevels = 1,
            .sampleCount = m_device.maxSampleCount,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .format = m_device.GetSurfaceFormat().format,
            .debugName = "color"};

        m_device.CreateTexture(m_colorImage, createInfo);
    }

    { // depth image
        TextureCreateInfo createInfo = {
            .width = m_device.GetSwapchainWidth(),
            .height = m_device.GetSwapchainHeight(),
            .mipLevels = 1,
            .sampleCount = m_device.maxSampleCount,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .format = VK_FORMAT_D32_SFLOAT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .debugName = "depth"};

        m_device.CreateTexture(m_depthImage, createInfo);
    }
}

void Renderer::DestroyAttachmentImages()
{
    m_device.WaitIdle();
    m_device.DestroyTexture(m_colorImage);
    m_device.DestroyTexture(m_depthImage);
}

void Renderer::CompileShaders()
{
    std::filesystem::create_directory(ShadersBinaryDir);
    for (auto &entry : std::filesystem::directory_iterator(ShadersDir)) {
        if (!entry.is_regular_file())
            continue;

        FilePath filepath = entry.path();
        String extension = filepath.extension();
        if (extension == ".vert" || extension == ".frag" || extension == ".comp" || extension == ".tesc" || extension == ".tese") {
            String filename = filepath.filename();

            // HACK: this uses command line to recompile all shaders
            String cmd = "glslangValidator -V " + filepath.string() + " -o " + String(ShadersBinaryDir / (filename + ".spv"));
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
            .sampleCount = m_device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB},
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = ReadFile(ShadersBinaryDir / "mesh.vert.spv", true),
            .fragmentCode = ReadFile(ShadersBinaryDir / "mesh.frag.spv", true),
        };

        m_device.CreateRenderPipeline(m_meshPipeline, createInfo);
    }

    { // skybox pipeline
        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants)}};

        RenderPipelineCreateInfo createInfo = {
            .pushConstantRanges = pushConstantRanges,
            .cullMode = VK_CULL_MODE_FRONT_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .sampleCount = m_device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB},
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = ReadFile(ShadersBinaryDir / "skybox.vert.spv", true),
            .fragmentCode = ReadFile(ShadersBinaryDir / "skybox.frag.spv", true),
        };

        m_device.CreateRenderPipeline(m_skyboxPipeline, createInfo);
    }

    { // shadowmap pipeline
        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DepthPushConstants)}};

        RenderPipelineCreateInfo createInfo = {
            .pushConstantRanges = pushConstantRanges,
            .cullMode = VK_CULL_MODE_NONE,
            .sampleCount = m_device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = ReadFile(ShadersBinaryDir / "shadowmap.vert.spv", true),
        };

        m_device.CreateRenderPipeline(m_shadowmapPipeline, createInfo);
    }
}

void Renderer::DestroyPipelines()
{
    m_device.DestroyRenderPipeline(m_meshPipeline);
    m_device.DestroyRenderPipeline(m_skyboxPipeline);
    m_device.DestroyRenderPipeline(m_shadowmapPipeline);
}

void Renderer::UpdateDynamicData()
{
    // lights
    UpdateGpuLights();
    if (m_gpuLights.size() > 0) {
        m_device.UploadBufferData(m_lightsBuffer, m_gpuLights.data(), sizeof(GPULight) * m_gpuLights.size());

        // HACK: uses first light
        UpdateShadowmapCascades(*m_camera, m_gpuLights[0].direction);
        m_device.UploadBufferData(m_cascadesBuffer, m_cascades.data(), m_cascades.size() * sizeof(Cascade));
    }

    // scene info
    SceneInfo sceneInfo = {};
    sceneInfo.view = m_camera->GetView();
    sceneInfo.projection = m_camera->GetProjection();
    sceneInfo.cameraPos = m_camera->GetPosition();
    sceneInfo.lightsCount = m_gpuLights.size();
    sceneInfo.skyboxTexID = gAssetManager->GetTextureByName("skybox")->GetID();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        m_device.UploadBufferData(m_sceneInfoBuffers[i], &sceneInfo, sizeof(sceneInfo));
}

void Renderer::UpdateGpuLights()
{
    m_gpuLights.clear();
}

void Renderer::UpdateShadowmapCascades(Camera &camera, vec3 lightDir)
{
    // references: https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/ and https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp
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
        m_cascades[i].depth = (camera.GetNearClip() + splitDist * clipRange) * -1.0f;
        m_cascades[i].viewProjection = lightOrthoMatrix * lightViewMatrix;

        lastSplitDist = cascadeSplits[i];
    }
}