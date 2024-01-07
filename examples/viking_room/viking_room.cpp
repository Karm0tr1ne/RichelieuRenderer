#include <array>
#include <stdexcept>
#include <iostream>
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VulkanApplicationBase.h"
#include "VulkanModel.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

class VikingRoom : public VulkanApplicationBase {
public:
    struct Textures {
        VulkanBase::Texture2D mainTexture;
    } textures;

    struct Meshes {
        Model vikingRoom;
    } models;

    struct {
        VulkanBase::VulkanBuffer uboMats;
    } uniformBuffer;

    struct {
        float xAngle;
        float yAngle;
        float zAngle;
    } guiParams;

    struct {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } mvpMatrices;

    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;

    VikingRoom() : VulkanApplicationBase() {
        title = "Richelieu - Basic Viking Room";
    }

    void prepare() override {
        VulkanApplicationBase::prepare();
        loadAssets();
        setupUniformBuffers();
        createDescriptorSetLayout();
        createPipelineLayout();
        createPipeline();
        createDescriptorSets();
        buildCommandBuffers();
    }

    void loadAssets() {
        models.vikingRoom.loadFromObj(VulkanBase::Tools::getAssetPath() + "viking_room/viking_room.obj", vulkanDevice);
        textures.mainTexture.loadFromFile(VulkanBase::Tools::getAssetPath() + "viking_room/viking_room.png", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice);
    }

    void updateUniformBuffers() {
        mvpMatrices.model = glm::rotate(glm::mat4(1.0f), guiParams.zAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        mvpMatrices.model = glm::rotate(mvpMatrices.model, guiParams.xAngle, glm::vec3(1.0f, 0.0f, 0.0f));
        mvpMatrices.model = glm::rotate(mvpMatrices.model, guiParams.yAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        mvpMatrices.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvpMatrices.proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 10.0f);
        mvpMatrices.proj[1][1] *= -1;

        memcpy(uniformBuffer.uboMats.mapped, &mvpMatrices, sizeof(mvpMatrices));
    }

    void setupUniformBuffers() {
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   &uniformBuffer.uboMats,
                                   sizeof(mvpMatrices));

        uniformBuffer.uboMats.map();

        updateUniformBuffers();
    }

    void createDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].pImmutableSamplers = nullptr;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].pImmutableSamplers = nullptr;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VK_CHECK_RESULT(vkCreateDescriptorSetLayout(vulkanDevice->logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout));
    }

    void createPipelineLayout() {
        VkPipelineLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        createInfo.pSetLayouts = &descriptorSetLayout;
        createInfo.setLayoutCount = 1;
        createInfo.pushConstantRangeCount = 0;
        createInfo.pPushConstantRanges = nullptr;

        VK_CHECK_RESULT(vkCreatePipelineLayout(vulkanDevice->logicalDevice, &createInfo, nullptr, &pipelineLayout));
    }

    void createPipeline() {
        auto bindingDescriptions = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState.vertexBindingDescriptionCount = 1;
        vertexInputState.pVertexBindingDescriptions = &bindingDescriptions;
        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

        std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
        };
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
        inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.flags = 0;
        inputAssemblyState.primitiveRestartEnable = VK_FALSE;
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.flags = 0;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.lineWidth = 1.0f;
        rasterizationState.depthBiasEnable = VK_FALSE;
        rasterizationState.rasterizerDiscardEnable = VK_FALSE;
        rasterizationState.depthBiasConstantFactor = 0.0f;
        rasterizationState.depthBiasClamp = 0.0f;
        rasterizationState.depthBiasSlopeFactor = 0.0f;

        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentState.blendEnable = VK_TRUE;
        colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = &colorBlendAttachmentState;

        VkPipelineDepthStencilStateCreateInfo depthStencilState{};
        depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilState.depthTestEnable = VK_TRUE;
        depthStencilState.depthWriteEnable = VK_TRUE;
        depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencilState.depthBoundsTestEnable = VK_FALSE;
        depthStencilState.stencilTestEnable = VK_FALSE;
        depthStencilState.minDepthBounds = 0.0f;
        depthStencilState.maxDepthBounds = 1.0f;
        depthStencilState.stencilTestEnable = VK_FALSE;
        depthStencilState.front = {};
        depthStencilState.back = {};

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;
        viewportState.flags = 0;

        VkPipelineMultisampleStateCreateInfo multisampleState{};
        multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleState.sampleShadingEnable = VK_FALSE;
        multisampleState.rasterizationSamples = vulkanDevice->msaaSamples;
        multisampleState.minSampleShading = 1.0f;
        multisampleState.pSampleMask = nullptr;
        multisampleState.alphaToCoverageEnable = VK_FALSE;
        multisampleState.alphaToOneEnable = VK_FALSE;

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {};
        shaderStages[0] = createShader(VulkanBase::Tools::getShaderPath() + "viking_room/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = createShader(VulkanBase::Tools::getShaderPath() + "viking_room/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        VkGraphicsPipelineCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfo.pInputAssemblyState = &inputAssemblyState;
        createInfo.pViewportState = &viewportState;
        createInfo.pRasterizationState = &rasterizationState;
        createInfo.pMultisampleState = &multisampleState;
        createInfo.pDepthStencilState = &depthStencilState;
        createInfo.pColorBlendState = &colorBlendState;
        createInfo.pDynamicState = &dynamicStateCreateInfo;
        createInfo.layout = pipelineLayout;
        createInfo.renderPass = renderPass;
        createInfo.subpass = 0;
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = -1;
        createInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        createInfo.pStages = shaderStages.data();
        createInfo.pVertexInputState = &vertexInputState;

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(vulkanDevice->logicalDevice, pipelineCache, 1, &createInfo, nullptr, &pipeline));
    }

    void createDescriptorSets() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 1;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 2;

        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        createInfo.pPoolSizes = poolSizes.data();
        createInfo.maxSets = 2;

        VK_CHECK_RESULT(vkCreateDescriptorPool(vulkanDevice->logicalDevice, &createInfo, nullptr, &descriptorPool));

        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &descriptorSetLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &allocateInfo, &descriptorSet));

        std::array<VkWriteDescriptorSet, 2> writeDescriptorSets{};
        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].dstSet = descriptorSet;
        writeDescriptorSets[0].dstBinding = 0;
        writeDescriptorSets[0].dstArrayElement = 0;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].pBufferInfo = &uniformBuffer.uboMats.bufferInfo;

        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].dstSet = descriptorSet;
        writeDescriptorSets[1].dstBinding = 1;
        writeDescriptorSets[1].dstArrayElement = 0;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].pImageInfo = &textures.mainTexture.imageInfo;

        vkUpdateDescriptorSets(vulkanDevice->logicalDevice, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0,
                               nullptr);
    }

    void buildCommandBuffers() override {
        VkCommandBufferBeginInfo bufferBeginInfo{};
        bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bufferBeginInfo.flags = 0;
        bufferBeginInfo.pInheritanceInfo = nullptr;

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = defaultClearColor;
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = {width, height};
        renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        for (uint32_t i = 0; i < drawCommandBuffers.size(); i++) {
            renderPassBeginInfo.framebuffer = frameBuffers[i];
            VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &bufferBeginInfo));
            vkCmdBeginRenderPass(drawCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(drawCommandBuffers[i], 0, 1, &models.vikingRoom.vertexBuffer.buffer, offsets);
            vkCmdBindIndexBuffer(drawCommandBuffers[i], models.vikingRoom.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            VkViewport viewport{};
            viewport.width = (float)width;
            viewport.height = (float)height;
            viewport.maxDepth = 1.0f;
            viewport.minDepth = 0.0f;
            vkCmdSetViewport(drawCommandBuffers[i], 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent = {width, height};
            scissor.offset = {0, 0};
            vkCmdSetScissor(drawCommandBuffers[i], 0, 1,&scissor);

            vkCmdBindPipeline(drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0,
                                    nullptr);

            vkCmdDrawIndexed(drawCommandBuffers[i], static_cast<uint32_t>(models.vikingRoom.indices.size()), 1, 0, 0, 0);
            showGUIWindow(drawCommandBuffers[i]);
            vkCmdEndRenderPass(drawCommandBuffers[i]);
            VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
        }
    }

    void drawFrame() override {
        VulkanApplicationBase::prepareFrame();
        buildCommandBuffers();
        updateUniformBuffers();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &drawCommandBuffers[currentBuffer];
        VK_CHECK_RESULT(vkQueueSubmit(vulkanDevice->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VulkanApplicationBase::submitFrame();
    }

    void showGUIWindow(VkCommandBuffer cmdBuffer) override {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("GUI", nullptr, ImGuiWindowFlags_MenuBar);
        ImGui::SetNextWindowSize(ImVec2(400, 300));
        if (ImGui::CollapsingHeader("Rotation Settings")) {
            ImGui::SliderAngle("Rotation X Axis", &guiParams.xAngle, 0);
            ImGui::SliderAngle("Rotation Y Axis", &guiParams.yAngle, 0);
            ImGui::SliderAngle("Rotation Z Axis", &guiParams.zAngle, 0);
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
    }

    ~VikingRoom() override {
        uniformBuffer.uboMats.cleanUp();
        models.vikingRoom.cleanUp();
        textures.mainTexture.cleanUp();

        vkDestroyPipeline(vulkanDevice->logicalDevice, pipeline, nullptr);

        vkDestroyPipelineLayout(vulkanDevice->logicalDevice, pipelineLayout, nullptr);
        vkDestroyDescriptorPool(vulkanDevice->logicalDevice, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(vulkanDevice->logicalDevice, descriptorSetLayout, nullptr);
    }
};

int main(int argc, char *argv[]) {
    auto *application = new VikingRoom();
    application->setupWindow();
    application->initVulkan();
    application->prepare();
    application->renderLoop();
    delete(application);
    return 0;
}
