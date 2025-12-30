#include "cycle/renderer.h"

#include "cycle/filesystem.h"
#include "cycle/graphics/render_device.h"
#include "cycle/graphics/vulkan_helpers.h"
#include "cycle/types/id.h"

#include "cycle/globals.h"

#include "cycle/types/material.h"
#include "cycle/types/render_data.h"

#include "cycle/types/shader_data.h"
#include <filesystem>

#include "imgui.h"
#include "imgui_impl_vulkan.h"

void Renderer::init(SDL_Window *window)
{
    assert(window);

    device.init(window);

    // init all resource managers
    g_textureManager->init(&device);
    g_meshManager->init(&device);
    g_materialManager->init();
    g_modelManager->init();

    createAttachmentImages();

    // create global data buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(SceneInfo),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        };

        bool created = device.createBuffer(sceneInfoBuffer, createInfo);
        assert(created);
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

            bool created = device.createSampler(linearSampler, samplerCreateInfo);
            assert(created);
        }

        { // nearest
            SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            bool created = device.createSampler(nearestSampler, samplerCreateInfo);
            assert(created);
        }
    }

    // create default resources
    {
        // create default texture
        auto texID = g_textureManager->createTexture(texturesDir / "checkerboard.png", "default");
        assert(texID != TextureID::Invalid && (uint32_t)texID == DEFAULT_TEXTURE_ID);

        // add default material
        g_materialManager->addMaterial(Material{.baseColorTexID = texID}, "default");
    }

    // create pipeline layout
    {
        const Vector<VkDescriptorSetLayoutBinding> bindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT}, // scene data
            {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024, VK_SHADER_STAGE_FRAGMENT_BIT}, // textures
            {2, VK_DESCRIPTOR_TYPE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT}, // samplers
            {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}, // materials
        };

        const Vector<VkPushConstantRange> pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshDrawInfo)}};

        PipelineLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.descriptorSetLayoutBindings = bindings;
        layoutCreateInfo.pushConstantRanges = pushConstantRanges;
        device.createPipelineLayout(geometryPipelineLayout, layoutCreateInfo);
    }
    createPipelines();

    // write static descriptors
    device.writeDescriptor(0, sceneInfoBuffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    device.writeDescriptor(2, linearSampler, VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_LINEAR_ID);
    device.writeDescriptor(2, nearestSampler, VK_DESCRIPTOR_TYPE_SAMPLER, SAMPLER_NEAREST_ID);
    device.updateDescriptors(geometryPipelineLayout);
}

void Renderer::shutdown()
{
    device.waitIdle();

    destroyAttachmentImages();

    g_textureManager->release();
    g_meshManager->release();

    device.destroyBuffer(sceneInfoBuffer);
    device.destroyBuffer(materialsBuffer);

    device.destroySampler(linearSampler);
    device.destroySampler(nearestSampler);

    device.destroyPipelineLayout(geometryPipelineLayout);
    device.destroyRenderPipeline(geometryPipeline);

    device.shutdown();
}

void Renderer::loadResources()
{
    auto &materials = g_materialManager->getMaterials();
    auto &textures = g_textureManager->getTextures();

    // create materials buffer
    if (materials.size() > 0) {
        if (materialsBuffer.size > 0) { // delete existing materials buffer
            device.destroyBuffer(materialsBuffer);
        }

        const BufferCreateInfo createInfo = {
            .size = sizeof(Material) * materials.size(),
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        bool created = device.createBuffer(materialsBuffer, createInfo);
        assert(created);

        device.uploadBufferData(materialsBuffer, materials.data(), createInfo.size);
    }

    // write dynamic descriptors
    for (size_t i = 0; i < textures.size(); i++) {
        auto &texture = textures[i];
        device.writeDescriptor(1, texture, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, i);
    }

    if (materials.size() > 0)
        device.writeDescriptor(3, materialsBuffer, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    device.updateDescriptors(geometryPipelineLayout);
}

void Renderer::reloadShaders()
{
    device.waitIdle();

    compileShaders();

    device.destroyRenderPipeline(geometryPipeline);
    createPipelines();
}

void Renderer::draw(World &world, Editor &editor)
{
    CommandEncoder cmdEncoder = {};
    if (!device.beginCommandBuffer(cmdEncoder)) {
        resizeWindow();
        return;
    }

    updateDynamicData();

    barriers.transitionImage(colorImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    barriers.transitionImage(depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    barriers.transitionImage(device.getSwapchainImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    barriers.flushBarriers(cmdEncoder.cmd);

    VkExtent2D renderArea = {device.getSwapchainWidth(), device.getSwapchainHeight()};
    VkRenderingAttachmentInfo depthAttachment = vulkan::createAttachmentInfo(depthImage.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, false, true);

    //===========================
    // Render meshes
    //===========================
    {
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::createAttachmentInfo(colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, false, true),
        };

        VkRenderingInfo renderingInfo = vulkan::createRenderingInfo(renderArea, colorAttachments, &depthAttachment);
        cmdEncoder.beginRendering(renderingInfo);

        cmdEncoder.bindPipeline(geometryPipeline);
        for (const RenderData &rd : world.getRenderData()) {
            Model *model = g_modelManager->getModelByID(rd.modelID);
            if (!model)
                continue;

            mat4 worldMatrix = rd.transform.getMatrix();
            for (MeshID meshID : model->meshes) {
                Mesh *mesh = g_meshManager->getMeshByID(meshID);
                if (!mesh)
                    continue;

                MeshDrawInfo push = {};
                push.worldMatrix = worldMatrix * mesh->worldMatrix;
                push.vertexBufferAddress = mesh->vertexBuffer.address;
                push.materialId = mesh->materialID != MaterialID::Invalid ? (unsigned int)mesh->materialID : DEFAULT_MATERIAL_ID;
                cmdEncoder.pushConstants(geometryPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &push, sizeof(push));

                cmdEncoder.bindIndexBuffer(mesh->indexBuffer);
                cmdEncoder.drawIndexed(mesh->indices.size(), 1, 0, 0, 0);
            }
        }
        cmdEncoder.endRendering();
    }

    //===========================
    // Render imgui
    //===========================
    {
        Vector<VkRenderingAttachmentInfo> colorAttachments = {
            vulkan::createAttachmentInfo(colorImage.view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, true, true, device.getSwapchainImageView(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
        };

        VkRenderingInfo renderingInfo = vulkan::createRenderingInfo(renderArea, colorAttachments, &depthAttachment);
        cmdEncoder.beginRendering(renderingInfo);

        editor.draw();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdEncoder.cmd);

        cmdEncoder.endRendering();
    }

    barriers.transitionImage(device.getSwapchainImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    barriers.flushBarriers(cmdEncoder.cmd);

    device.endCommandBuffer(cmdEncoder);

    if (!device.swapchainPresent()) {
        resizeWindow();
        return;
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
            .format = VK_FORMAT_B8G8R8A8_SRGB,
        };

        bool created = device.createImage(colorImage, createInfo);
        assert(created);
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
        };

        bool created = device.createImage(depthImage, createInfo);
        assert(created);
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
    const RenderPipelineCreateInfo createInfo = {
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .sampleCount = device.maxSampleCount,
        .depthCompareOp = VK_COMPARE_OP_GREATER,
        .depthWriteEnable = true,
        .colorAttachmentFormats = {VK_FORMAT_B8G8R8A8_SRGB},
        .depthAttachmentFormat = VK_FORMAT_D32_SFLOAT,
        .pipelineLayout = &geometryPipelineLayout,
        .vertexCode = filesystem::readBinaryFile(shadersBinaryDir / "mesh.vert.spv"),
        .fragmentCode = filesystem::readBinaryFile(shadersBinaryDir / "mesh.frag.spv"),
    };
    device.createRenderPipeline(geometryPipeline, createInfo);
}

void Renderer::updateDynamicData()
{
    assert(camera && "Camera is not set!");
    if (void *mapped = sceneInfoBuffer.allocation.info.pMappedData; mapped != nullptr) {
        SceneInfo sceneInfo = {};
        sceneInfo.viewProjection = camera->getProjection() * camera->getView();
        memcpy(mapped, &sceneInfo, sizeof(sceneInfo));
    }
}