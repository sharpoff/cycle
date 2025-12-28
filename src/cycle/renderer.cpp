#include "cycle/renderer.h"

#include "cycle/filesystem.h"
#include "cycle/graphics/graphics_types.h"
#include "cycle/graphics/render_device.h"
#include "cycle/types/id.h"

#include "cycle/globals.h"
#include "cycle/resource_manager.h"

#include "cycle/types/material.h"
#include "imgui.h"

#include "cycle/types/shader_data.h"
#include <filesystem>

void Renderer::init(SDL_Window *window)
{
    assert(window);

    device.init(window);

    ResourceManager &resourceManager = g_engine->getResourceManager();
    resourceManager.setRenderDevice(&device);

    createAttachmentImages();

    // create global data buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(SceneInfo),
            .usage = BUFFER_USAGE_UNIFORM,
        };

        bool created = device.createBuffer(sceneInfoBuffer, createInfo);
        assert(created);
    }

    // create common samplers
    {
        { // linear
            SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter = SAMPLER_FILTER_LINEAR;
            samplerCreateInfo.minFilter = SAMPLER_FILTER_LINEAR;
            samplerCreateInfo.addressModeU = SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeV = SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeW = SAMPLER_ADDRESS_MODE_REPEAT;

            bool created = device.createSampler(linearSampler, samplerCreateInfo);
            assert(created);
        }

        { // nearest
            SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter = SAMPLER_FILTER_NEAREST;
            samplerCreateInfo.minFilter = SAMPLER_FILTER_NEAREST;
            samplerCreateInfo.addressModeU = SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeV = SAMPLER_ADDRESS_MODE_REPEAT;
            samplerCreateInfo.addressModeW = SAMPLER_ADDRESS_MODE_REPEAT;

            bool created = device.createSampler(nearestSampler, samplerCreateInfo);
            assert(created);
        }
    }

    // create default resources
    {
        // create default texture
        auto texID = resourceManager.loadTextureFromFile("default", texturesDir / "checkerboard.png");
        assert(texID != TextureID::Invalid && (uint32_t)texID == DEFAULT_TEXTURE_ID);

        // create default material
        auto matID = resourceManager.addMaterial("default", {.baseColorTexID = texID});
        assert(matID != MaterialID::Invalid && (uint32_t)matID == DEFAULT_MATERIAL_ID);
    }

    // create pipeline layout
    {
        const Vector<DescriptorSetLayoutBinding> bindings0 = {
            {0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_VERTEX}, // scene data
            {1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024, SHADER_STAGE_FRAGMENT}, // textures
            {2, DESCRIPTOR_TYPE_SAMPLER, 2, SHADER_STAGE_FRAGMENT}, // samplers
            {3, DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, SHADER_STAGE_FRAGMENT}, // materials
        };

        const Vector<DescriptorSetLayout> descriptorSetLayouts = {
            {bindings0}, // set 0
        };

        const Vector<PushConstantRange> pushConstantRanges = {
            {SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, 0, sizeof(MeshDrawInfo)}};

        PipelineLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.descriptorSetLayouts = descriptorSetLayouts;
        layoutCreateInfo.pushConstantRanges = pushConstantRanges;
        device.createPipelineLayout(geometryPipelineLayout, layoutCreateInfo);
    }
    createPipelines();

    // write static descriptors
    device.writeDescriptor(0, sceneInfoBuffer, DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    device.writeDescriptor(2, linearSampler, DESCRIPTOR_TYPE_SAMPLER, SAMPLER_LINEAR_ID);
    device.writeDescriptor(2, nearestSampler, DESCRIPTOR_TYPE_SAMPLER, SAMPLER_NEAREST_ID);

    device.updateDescriptors(geometryPipelineLayout, bindlessDescriptorSet);
}

void Renderer::shutdown()
{
    device.waitIdle();

    destroyAttachmentImages();

    ResourceManager &resourceManager = g_engine->getResourceManager();
    resourceManager.destroyAllResources();

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
    ResourceManager &resourceManager = g_engine->getResourceManager();
    auto &materials = resourceManager.getMaterials();
    auto &textures = resourceManager.getTextures();

    // create materials buffer
    if (materials.size() > 0) {
        if (materialsBuffer.size > 0) { // delete existing materials buffer
            device.destroyBuffer(materialsBuffer);
        }

        const BufferCreateInfo createInfo = {
            .size = sizeof(Material) * materials.size(),
            .usage = BUFFER_USAGE_STORAGE | BUFFER_USAGE_TRANSFER_DST,
        };

        bool created = device.createBuffer(materialsBuffer, createInfo);
        assert(created);

        device.uploadBufferData(materialsBuffer, resourceManager.getMaterials().data(), createInfo.size);
    }

    // write dynamic descriptors
    for (size_t i = 0; i < textures.size(); i++) {
        auto &texture = textures[i];
        device.writeDescriptor(1, texture, DESCRIPTOR_TYPE_SAMPLED_IMAGE, i);
    }

    if (materials.size() > 0)
        device.writeDescriptor(3, materialsBuffer, DESCRIPTOR_TYPE_STORAGE_BUFFER);

    device.updateDescriptors(geometryPipelineLayout, bindlessDescriptorSet);
}

void Renderer::recreatePipelines()
{
    device.waitIdle();

    device.destroyRenderPipeline(geometryPipeline);

    compileShaders();
    createPipelines();
}

void Renderer::draw()
{
    CommandEncoder cmdEncoder = {};
    if (!device.beginCommandEncoding(cmdEncoder)) {
        // recreate all swapchain dependant resources
        destroyAttachmentImages();
        createAttachmentImages();
        return;
    }

    updateDynamicData();

    AttachmentInfo swapchainAttachment = {
        .image = &device.getSwapchainImage(),
        .load = false,
        .store = true,
    };

    AttachmentInfo depthAttachment = {
        .image = &depthImage,
        .load = false,
        .store = true,
    };

    cmdEncoder.beginRendering({
        .renderAreaExtent = {device.getSwapchainWidth(), device.getSwapchainHeight()},
        .colorAttachments = {swapchainAttachment},
        .depthAttachment = &depthAttachment,
    });

    ResourceManager &resourceManager = g_engine->getResourceManager();

    cmdEncoder.bindPipeline(geometryPipeline);
    if (Model *model = resourceManager.getModelByName("sponza"); model != nullptr) {
        for (MeshID meshID : model->meshes) {
            if (meshID == MeshID::Invalid)
                continue;

            Mesh *mesh = resourceManager.getMeshByID(meshID);
            if (!mesh)
                continue;

            MeshDrawInfo push = {};
            push.worldMatrix = model->worldMatrix * mesh->worldMatrix;
            push.vertexBufferAddress = mesh->vertexBuffer.address;
            push.materialId = mesh->materialID != MaterialID::Invalid ? (unsigned int)mesh->materialID : DEFAULT_MATERIAL_ID;
            cmdEncoder.pushConstants(geometryPipelineLayout, SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT, &push, sizeof(push));

            cmdEncoder.bindIndexBuffer(mesh->indexBuffer);
            cmdEncoder.drawIndexed(mesh->indices.size(), 1, 0, 0, 0);
        }
    }

    // render imgui
    cmdEncoder.beginImGuiFrame();
    ImGui::ShowDemoWindow();
    cmdEncoder.endImGuiFrame();

    cmdEncoder.endRendering();
    device.endCommandEncoding(cmdEncoder);

    if (!device.swapchainPresent()) {
        // recreate all swapchain dependant resources
        destroyAttachmentImages();
        createAttachmentImages();
        return;
    }
}

void Renderer::createAttachmentImages()
{
    { // depth image
        ImageCreateInfo createInfo = {
            .width = device.getSwapchainWidth(),
            .height = device.getSwapchainHeight(),
            .mipLevels = 1,
            .sampleCount = 1,
            .usage = IMAGE_USAGE_DEPTH_ATTACHMENT,
            .format = IMAGE_FORMAT_D32_SFLOAT,
        };

        bool created = device.createImage(depthImage, createInfo, "depth image");
        assert(created);
    }
}

void Renderer::destroyAttachmentImages()
{
    device.waitIdle();
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
        .cullMode = CULL_MODE_BACK,
        .frontFace = FRONT_FACE_COUNTER_CLOCKWISE,
        .depthCompareOp = COMPARE_OP_GREATER,
        .depthWriteEnable = true,
        .colorAttachmentFormats = {IMAGE_FORMAT_B8G8R8A8_SRGB},
        .depthAttachmentFormat = IMAGE_FORMAT_D32_SFLOAT,
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

uint32_t Renderer::calculateMipLevels(uint32_t width, uint32_t height)
{
    return floor(log2(std::max(width, height))) + 1;
}