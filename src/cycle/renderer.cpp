#include "cycle/renderer.h"

#include <filesystem>

#include "cycle/editor.h"
#include "cycle/filesystem.h"
#include "cycle/graphics/render_device.h"
#include "cycle/graphics/vulkan_helpers.h"
#include "cycle/types/gpu_light.h"
#include "cycle/types/id.h"
#include "cycle/types/push_constants.h"
#include "cycle/types/scene_info.h"
#include "cycle/types/material.h"

#include "cycle/managers/texture_manager.h"
#include "cycle/managers/model_manager.h"
#include "cycle/managers/material_manager.h"
#include "cycle/managers/entity_manager.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

Renderer *g_renderer;

void Renderer::init(SDL_Window *window)
{
    static Renderer instance;
    g_renderer = &instance;

    instance.initInternal(window);
}

Renderer *Renderer::get()
{
    return g_renderer;
}

void Renderer::initInternal(SDL_Window *window)
{
    assert(window);

    this->window = window;
    device.init(window);

    // init all resource managers
    TextureManager::init(&device);
    ModelManager::init(&device);
    MaterialManager::init();

    createAttachmentImages();

    { // shadowmap image
        ImageCreateInfo createInfo = {
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
            shadowmapImages[i] = device.createImage(createInfo);
        }
    }

    // create global data buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(SceneInfo),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
            sceneInfoBuffers[i] = device.createBuffer(createInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }

    // create cascades matrices buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(Cascade) * cascades.size(),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };

        cascadesBuffer = device.createBuffer(createInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    // create common samplers
    {
        { // linear
            SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.maxLod = 4;

            linearSampler = device.createSampler(samplerCreateInfo);
        }

        { // nearest
            SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.maxLod = 4;

            nearestSampler = device.createSampler(samplerCreateInfo);
        }
    }

    // create default resources
    {
        // create default texture
        auto texID = TextureManager::get()->createTexture(texturesDir / "compressed/checkerboard.ktx", VK_FORMAT_R8G8B8A8_SRGB, "default");
        assert(texID != TextureID::Invalid);

        // add default material
        MaterialManager::get()->addMaterial(Material{.baseColorTexID = texID}, "default");
    }

    createPipelines();

    // write static descriptors
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        device.writeDescriptor(0, sceneInfoBuffers[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }
    device.writeDescriptor(2, linearSampler, VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_LINEAR_ID);
    device.writeDescriptor(2, nearestSampler, VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_NEAREST_ID);

    device.updateDescriptors();
}

void Renderer::shutdown()
{
    device.waitIdle();

    destroyAttachmentImages();
    
    for (auto &shadowmap : shadowmapImages) {
        device.destroyImage(shadowmap);
    }

    TextureManager::get()->release();
    ModelManager::get()->release();

    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        device.destroyBuffer(sceneInfoBuffers[i]);
    }
    device.destroyBuffer(materialsBuffer);
    device.destroyBuffer(lightsBuffer);
    device.destroyBuffer(cascadesBuffer);

    device.destroySampler(linearSampler);
    device.destroySampler(nearestSampler);

    destroyPipelines();

    device.shutdown();
}

void Renderer::loadDynamicResources()
{
    auto &materials = MaterialManager::get()->getMaterials();
    auto &textures = TextureManager::get()->getTextures();
    lightEntities = EntityManager::get()->lights.getEntities();

    // create materials buffer
    if (materials.size() > 0) {
        if (materialsBuffer.size > 0) { // delete existing materials buffer
            device.destroyBuffer(materialsBuffer);
        }

        const BufferCreateInfo createInfo = {
            .size = sizeof(Material) * materials.size(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        materialsBuffer = device.createBuffer(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        device.uploadBufferData(materialsBuffer, materials.data(), createInfo.size);
    }

    // create lights buffer
    if (lightEntities.size() > 0) {
        if (lightsBuffer.size > 0) { // delete existing lights buffer
            device.destroyBuffer(lightsBuffer);
        }

        updateGPULights();

        const BufferCreateInfo createInfo = {
            .size = sizeof(GPULight) * gpuLights.size(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        lightsBuffer = device.createBuffer(createInfo, VMA_MEMORY_USAGE_CPU_TO_GPU);
        device.uploadBufferData(lightsBuffer, gpuLights.data(), createInfo.size);
    }

    // write dynamic descriptors
    for (size_t i = 0; i < textures.size(); i++) {
        auto &texture = textures[i];
        device.writeDescriptor(1, texture, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, i);
    }

    if (materials.size() > 0)
        device.writeDescriptor(3, materialsBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    if (lightEntities.size() > 0)
        device.writeDescriptor(4, lightsBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    device.writeDescriptor(5, cascadesBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    device.updateDescriptors();
}

void Renderer::reloadShaders()
{
    device.waitIdle();

    compileShaders();

    destroyPipelines();
    createPipelines();
}

void Renderer::draw()
{
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (cmd = device.beginCommandBuffer(); cmd == VK_NULL_HANDLE) {
        resizeWindow();
        return;
    }

    update();

    barriers.transitionImage2(colorImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.transitionImage2(depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    for (auto &shadowmap : shadowmapImages)
        barriers.transitionImage2(shadowmap.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    barriers.transitionImage2(device.getSwapchainImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.flushBarriers(cmd);

    const uint32_t currentFrame = device.getCurrentFrame();
    VkExtent2D renderArea = {device.getSwapchainWidth(), device.getSwapchainHeight()};
    const float color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    VkRenderingAttachmentInfo depthAttachment = vulkan::createAttachmentInfo(depthImage.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false, true);

    //===========================
    // Render skybox
    //===========================
    {
        vulkan::beginDebugLabel(cmd, "skybox");
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::createAttachmentInfo(colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false, true),
        };
        VkRenderingInfo renderingInfo = vulkan::createRenderingInfo(renderArea, colorAttachments, &depthAttachment);
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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline.layout, 0, 1, &device.getBindlessDescriptor(), 0, 0);

        Model *model = ModelManager::get()->getModelByName("cube");
        drawModel(cmd, model, mat4(1.0f));

        vkCmdEndRendering(cmd);
        vulkan::endDebugLabel(cmd);
    }

    //===========================
    // Render shadowmap
    //===========================
    vulkan::beginDebugLabel(cmd, "shadowmapping");
    for (uint32_t i = 0; i < shadowmapImages.size(); i++)
    {
        Image &shadowmap = shadowmapImages[i];

        VkRenderingAttachmentInfo shadowmapDepthAttachment = vulkan::createAttachmentInfo(shadowmap.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false, true);

        VkRenderingInfo renderingInfo = vulkan::createRenderingInfo({shadowmap.width, shadowmap.height}, {}, &shadowmapDepthAttachment);
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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmapPipeline.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowmapPipeline.layout, 0, 1, &device.getBindlessDescriptor(), 0, 0);

        for (EntityID entity : EntityManager::get()->models.getEntities()) {
            ModelComponent    *modelComponent = EntityManager::get()->models.getComponent(entity);
            TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(entity);
            if (!modelComponent || !transformComponent)
                continue;

            Model *model = ModelManager::get()->getModelByID(modelComponent->modelID);
            if (!model) continue;

            for (Mesh &mesh : model->meshes) {
                DepthPushConstants push = {};
                push.worldMatrix = transformComponent->transform * mesh.worldMatrix;
                push.vertexBufferAddress = mesh.vertexBuffer.address;
                push.cascadeIndex = i;
                vkCmdPushConstants(cmd, shadowmapPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);

                vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, mesh.indices.size(), 1, 0, 0, 0);
            }
        }
        vkCmdEndRendering(cmd);
    }
    vulkan::endDebugLabel(cmd);

    //===========================
    // Render meshes
    //===========================
    {
        vulkan::beginDebugLabel(cmd, "mesh");
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::createAttachmentInfo(colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true, true),
        };

        VkRenderingInfo renderingInfo = vulkan::createRenderingInfo(renderArea, colorAttachments, &depthAttachment);
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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline.pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline.layout, 0, 1, &device.getBindlessDescriptor(), 0, 0);

        for (EntityID entity : EntityManager::get()->models.getEntities()) {
            ModelComponent    *modelComponent = EntityManager::get()->models.getComponent(entity);
            TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(entity);
            if (!modelComponent || !transformComponent)
                continue;

            Model *model = ModelManager::get()->getModelByID(modelComponent->modelID);
            drawModel(cmd, model, transformComponent->transform);
        }
        vkCmdEndRendering(cmd);
        vulkan::endDebugLabel(cmd);
    }

    //===========================
    // Render imgui
    //===========================
    {
        vulkan::beginDebugLabel(cmd, "imgui");
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::createAttachmentInfo(colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true, true, device.getSwapchainImageView(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
        };

        VkRenderingInfo renderingInfo = vulkan::createRenderingInfo(renderArea, colorAttachments, &depthAttachment);
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

        Editor::get()->draw();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        vkCmdEndRendering(cmd);
        vulkan::endDebugLabel(cmd);
    }

    barriers.transitionImage2(device.getSwapchainImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);
    barriers.flushBarriers(cmd);

    device.endCommandBuffer(cmd);

    if (!device.swapchainPresent()) {
        resizeWindow();
        return;
    }
}

void Renderer::drawModel(VkCommandBuffer cmd, Model *model, mat4 worldMatrix)
{
    if (!model) {
        LOGE("%s", "Failed to draw model");
        return;
    }

    for (Mesh &mesh : model->meshes) {
        MeshPushConstants push = {};
        push.worldMatrix = worldMatrix * mesh.worldMatrix;
        push.vertexBufferAddress = mesh.vertexBuffer.address;
        push.materialId = (unsigned int)mesh.materialID;
        vkCmdPushConstants(cmd, meshPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);

        vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh.indices.size(), 1, 0, 0, 0);
    }
}

void Renderer::resizeWindow()
{
    // recreate all swapchain dependant resources
    destroyAttachmentImages();
    createAttachmentImages();

    camera->setAspectRatio((float)device.getSwapchainWidth() / device.getSwapchainHeight());
}

void Renderer::createAttachmentImages()
{
    { // color image
        ImageCreateInfo createInfo = {
            .width = device.getSwapchainWidth(),
            .height = device.getSwapchainHeight(),
            .mipLevels = 1,
            .sampleCount = device.maxSampleCount,
            .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .format = device.getSurfaceFormat().format,
            .debugName = "color"
        };

        colorImage = device.createImage(createInfo);
    }

    { // depth image
        ImageCreateInfo createInfo = {
            .width = device.getSwapchainWidth(),
            .height = device.getSwapchainHeight(),
            .mipLevels = 1,
            .sampleCount = device.maxSampleCount,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .format = VK_FORMAT_D32_SFLOAT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT,
            .debugName = "depth"
        };

        depthImage = device.createImage(createInfo);
    }
}

void Renderer::destroyAttachmentImages()
{
    device.waitIdle();
    device.destroyImage(colorImage);
    device.destroyImage(depthImage);
}

void Renderer::compileShaders()
{
    std::filesystem::create_directory(shadersBinaryDir);
    for (auto &entry : std::filesystem::directory_iterator(shadersDir)) {
        if (!entry.is_regular_file())
            continue;

        std::filesystem::path filepath = entry.path();
        String extension = filepath.extension();
        if (extension == ".vert" || extension == ".frag" || extension == ".comp" || extension == ".tesc" || extension == ".tese") {
            String filename = filepath.filename();

            String cmd = "glslangValidator -V " + filepath.string() + " -o " + String(shadersBinaryDir / (filename + ".spv"));
            system(cmd.c_str());
        }
    }
}

void Renderer::createPipelines()
{
    { // mesh pipeline
        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants)}
        };

        RenderPipelineCreateInfo createInfo = {
            .pushConstantRanges = pushConstantRanges,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .sampleCount = device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB},
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = filesystem::readFile(shadersBinaryDir / "mesh.vert.spv", true),
            .fragmentCode = filesystem::readFile(shadersBinaryDir / "mesh.frag.spv", true),
        };

        meshPipeline = device.createRenderPipeline(createInfo);
    }

    { // skybox pipeline
        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants)}
        };

        RenderPipelineCreateInfo createInfo = {
            .pushConstantRanges = pushConstantRanges,
            .cullMode = VK_CULL_MODE_FRONT_BIT,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .sampleCount = device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB},
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = filesystem::readFile( shadersBinaryDir / "skybox.vert.spv", true),
            .fragmentCode = filesystem::readFile( shadersBinaryDir / "skybox.frag.spv", true),
        };

        skyboxPipeline = device.createRenderPipeline(createInfo);
    }

    { // shadowmap pipeline
        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(DepthPushConstants)}
        };

        RenderPipelineCreateInfo createInfo = {
            .pushConstantRanges = pushConstantRanges,
            .cullMode = VK_CULL_MODE_NONE,
            .sampleCount = device.maxSampleCount,
            .depthCompareOp = VK_COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
            .vertexCode = filesystem::readFile(shadersBinaryDir / "shadowmap.vert.spv", true),
        };

        shadowmapPipeline = device.createRenderPipeline(createInfo);
    }
}

void Renderer::destroyPipelines()
{
    device.destroyRenderPipeline(meshPipeline);
    device.destroyRenderPipeline(skyboxPipeline);
    device.destroyRenderPipeline(shadowmapPipeline);
}

void Renderer::update()
{
    assert(camera && "Camera is not set!");

    updateGPULights();
    if (gpuLights.size() > 0) {
        device.uploadBufferData(lightsBuffer, gpuLights.data(), sizeof(GPULight) * gpuLights.size());

        // HACK
        updateShadowmapCascades(gpuLights[0].direction);
        device.uploadBufferData(cascadesBuffer, cascades.data(), cascades.size() * sizeof(Cascade));
    }

    SceneInfo sceneInfo = {};
    sceneInfo.view = camera->getView();
    sceneInfo.projection = camera->getProjection();
    sceneInfo.cameraPos = camera->getPosition();
    sceneInfo.lightsCount = gpuLights.size();
    sceneInfo.skyboxTexID = (uint32_t)TextureManager::get()->getTextureIDByName("skybox");
    
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++)
        device.uploadBufferData(sceneInfoBuffers[i], &sceneInfo, sizeof(sceneInfo));
}

void Renderer::updateGPULights()
{
    gpuLights.clear();
    for (EntityID lightID : lightEntities) {
        LightComponent     *lightComponent = EntityManager::get()->lights.getComponent(lightID);
        TransformComponent *transformComponent = EntityManager::get()->transforms.getComponent(lightID);
        if (!lightComponent || !transformComponent)
            continue;

        GPULight &light = gpuLights.emplace_back();
        light.intensity = 1.0f;
        light.position = math::getPosition(transformComponent->transform);
        light.direction = lightComponent->direction;
        light.color = lightComponent->color;
        light.type = lightComponent->lightType;
    }
}

// references: https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
//          and https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmappingcascade/shadowmappingcascade.cpp
void Renderer::updateShadowmapCascades(vec3 lightDir)
{
    float cascadeSplits[SHADOWMAP_CASCADES];

    float nearClip = camera->getNearClip();
    float farClip = camera->getFarClip();
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
        glm::mat4 invCam = glm::inverse(camera->getProjection() * camera->getView());

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
        cascades[i].depth = (camera->getNearClip() + splitDist * clipRange) * -1.0f;
        cascades[i].viewProjection = lightOrthoMatrix * lightViewMatrix;

        lastSplitDist = cascadeSplits[i];
    }
}