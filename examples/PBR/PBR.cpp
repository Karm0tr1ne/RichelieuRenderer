
#include <array>
#include <stdexcept>
#include <iostream>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "VulkanApplicationBase.h"
#include "VulkanModel.h"
#include "VulkanBuffer.h"
#include "VulkanTexture.h"

class PbrExample : public VulkanApplicationBase {
public:
    bool displaySkybox = false;

    struct Textures {
        VulkanBase::Texture2D mainTex;
        VulkanBase::Texture2D emissionMap;
        VulkanBase::Texture2D metallicMap;
        VulkanBase::Texture2D normalMap;
        VulkanBase::Texture2D occlusionMap;
        VulkanBase::Texture2D roughnessMap;
        VulkanBase::TextureCubeMap envMap;
    } textures;

    struct Meshes {
        Model helmet;
        Model envCube;
    } models;

    struct {
        VulkanBase::VulkanBuffer object;
        VulkanBase::VulkanBuffer skybox;
        VulkanBase::VulkanBuffer uboParams;
    } uniformBuffers;

    struct {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
        glm::vec3 camPos;
    } mvpMatrices;

    struct UBOParams {
        glm::vec4 lightPos;
        float exposure = 4.5f;
        float gamma = 2.2f;
    } uboParams;

    VkPipelineLayout pipelineLayout;

    struct {
        VkPipeline pbr;
        VkPipeline skybox;
    } pipelines;

    struct {
        VkDescriptorSet pbr;
        VkDescriptorSet skybox;
    } descriptorSets;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;

    PbrExample() : VulkanApplicationBase() {
        title = "Richelieu - PBR Example";

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
        models.helmet.loadFromObj(VulkanBase::Tools::getAssetPath() + "PBR/helmet.obj", vulkanDevice);
        models.envCube.loadFromObj(VulkanBase::Tools::getAssetPath() + "skybox/cube.obj", vulkanDevice);
        textures.mainTex.loadFromFile(VulkanBase::Tools::getAssetPath() + "PBR/helmet_basecolor.tga", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice);
        textures.emissionMap.loadFromFile(VulkanBase::Tools::getAssetPath() + "PBR/helmet_emission.tga", VK_FORMAT_R8G8B8A8_SNORM, vulkanDevice);
        textures.metallicMap.loadFromFile(VulkanBase::Tools::getAssetPath() + "PBR/helmet_metalness.tga", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice);
        textures.normalMap.loadFromFile(VulkanBase::Tools::getAssetPath() + "PBR/helmet_normal.tga", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice);
        textures.occlusionMap.loadFromFile(VulkanBase::Tools::getAssetPath() + "PBR/helmet_occlusion.tga", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice);
        textures.roughnessMap.loadFromFile(VulkanBase::Tools::getAssetPath() + "PBR/helmet_roughness.tga", VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice);
        std::array<std::string, 6> filePaths = {
                VulkanBase::Tools::getAssetPath() + "skybox/right.jpg",
                VulkanBase::Tools::getAssetPath() + "skybox/left.jpg",
                VulkanBase::Tools::getAssetPath() + "skybox/top.jpg",
                VulkanBase::Tools::getAssetPath() + "skybox/bottom.jpg",
                VulkanBase::Tools::getAssetPath() + "skybox/front.jpg",
                VulkanBase::Tools::getAssetPath() + "skybox/back.jpg",
        };
        textures.envMap.loadFromFiles(filePaths, VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice);
    }

    void updateUniformBuffers() {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        mvpMatrices.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvpMatrices.model = glm::rotate(mvpMatrices.model, 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        mvpMatrices.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        mvpMatrices.proj = glm::perspective(glm::radians(45.0f), (float) width / (float) height, 0.1f, 10.0f);
        // flip Y axis
        mvpMatrices.proj[1][1] *= -1;
        mvpMatrices.camPos = glm::vec3(0, 0, 0);
        memcpy(uniformBuffers.object.mapped, &mvpMatrices, sizeof(mvpMatrices));

        memcpy(uniformBuffers.skybox.mapped, &mvpMatrices, sizeof(mvpMatrices));
    }

    void updateUBOParams() {
        uboParams.lightPos = glm::vec4(-15.0f, -7.5f, -15.0f, 1.0f);

        memcpy(uniformBuffers.uboParams.mapped, &uboParams, sizeof(uboParams));
    }

    void setupUniformBuffers() {
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   &uniformBuffers.object,
                                   sizeof(mvpMatrices));
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   &uniformBuffers.skybox,
                                   sizeof(mvpMatrices));
        vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                   &uniformBuffers.uboParams,
                                   sizeof(uboParams));

        uniformBuffers.object.map();
        uniformBuffers.skybox.map();
        uniformBuffers.uboParams.map();

        updateUniformBuffers();
        updateUBOParams();
    }

    void createDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 8> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].pImmutableSamplers = nullptr;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].pImmutableSamplers = nullptr;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[2].binding = 2;
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].pImmutableSamplers = nullptr;
        bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[3].binding = 3;
        bindings[3].descriptorCount = 1;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[3].pImmutableSamplers = nullptr;
        bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[4].binding = 4;
        bindings[4].descriptorCount = 1;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[4].pImmutableSamplers = nullptr;
        bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[5].binding = 5;
        bindings[5].descriptorCount = 1;
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[5].pImmutableSamplers = nullptr;
        bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[6].binding = 6;
        bindings[6].descriptorCount = 1;
        bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[6].pImmutableSamplers = nullptr;
        bindings[6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[7].binding = 7;
        bindings[7].descriptorCount = 1;
        bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[7].pImmutableSamplers = nullptr;
        bindings[7].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        shaderStages[0] = createShader(VulkanBase::Tools::getShaderPath() + "PBR/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = createShader(VulkanBase::Tools::getShaderPath() + "PBR/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(vulkanDevice->logicalDevice, pipelineCache, 1, &createInfo, nullptr, &pipelines.pbr));

        rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
        shaderStages[0] = createShader(VulkanBase::Tools::getShaderPath() + "skybox/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        shaderStages[1] = createShader(VulkanBase::Tools::getShaderPath() + "skybox/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        depthStencilState.depthWriteEnable = VK_FALSE;
        depthStencilState.depthTestEnable = VK_FALSE;
        VK_CHECK_RESULT(vkCreateGraphicsPipelines(vulkanDevice->logicalDevice, pipelineCache, 1, &createInfo, nullptr, &pipelines.skybox));
    }

    void createDescriptorSets() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = 4;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = 16;

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

        // Main Object
        VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &allocateInfo, &descriptorSets.pbr));

        std::array<VkWriteDescriptorSet, 8> objectDescriptorSets{};
        objectDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectDescriptorSets[0].dstSet = descriptorSets.pbr;
        objectDescriptorSets[0].dstBinding = 0;
        objectDescriptorSets[0].dstArrayElement = 0;
        objectDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        objectDescriptorSets[0].descriptorCount = 1;
        objectDescriptorSets[0].pBufferInfo = &uniformBuffers.object.bufferInfo;

        objectDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectDescriptorSets[1].dstSet = descriptorSets.pbr;
        objectDescriptorSets[1].dstBinding = 1;
        objectDescriptorSets[1].dstArrayElement = 0;
        objectDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        objectDescriptorSets[1].descriptorCount = 1;
        objectDescriptorSets[1].pBufferInfo = &uniformBuffers.uboParams.bufferInfo;

        objectDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectDescriptorSets[2].dstSet = descriptorSets.pbr;
        objectDescriptorSets[2].dstBinding = 2;
        objectDescriptorSets[2].dstArrayElement = 0;
        objectDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        objectDescriptorSets[2].descriptorCount = 1;
        objectDescriptorSets[2].pImageInfo = &textures.mainTex.imageInfo;

        objectDescriptorSets[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectDescriptorSets[3].dstSet = descriptorSets.pbr;
        objectDescriptorSets[3].dstBinding = 3;
        objectDescriptorSets[3].dstArrayElement = 0;
        objectDescriptorSets[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        objectDescriptorSets[3].descriptorCount = 1;
        objectDescriptorSets[3].pImageInfo = &textures.normalMap.imageInfo;

        objectDescriptorSets[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectDescriptorSets[4].dstSet = descriptorSets.pbr;
        objectDescriptorSets[4].dstBinding = 4;
        objectDescriptorSets[4].dstArrayElement = 0;
        objectDescriptorSets[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        objectDescriptorSets[4].descriptorCount = 1;
        objectDescriptorSets[4].pImageInfo = &textures.metallicMap.imageInfo;

        objectDescriptorSets[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectDescriptorSets[5].dstSet = descriptorSets.pbr;
        objectDescriptorSets[5].dstBinding = 5;
        objectDescriptorSets[5].dstArrayElement = 0;
        objectDescriptorSets[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        objectDescriptorSets[5].descriptorCount = 1;
        objectDescriptorSets[5].pImageInfo = &textures.roughnessMap.imageInfo;

        objectDescriptorSets[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectDescriptorSets[6].dstSet = descriptorSets.pbr;
        objectDescriptorSets[6].dstBinding = 6;
        objectDescriptorSets[6].dstArrayElement = 0;
        objectDescriptorSets[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        objectDescriptorSets[6].descriptorCount = 1;
        objectDescriptorSets[6].pImageInfo = &textures.occlusionMap.imageInfo;

        objectDescriptorSets[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        objectDescriptorSets[7].dstSet = descriptorSets.pbr;
        objectDescriptorSets[7].dstBinding = 7;
        objectDescriptorSets[7].dstArrayElement = 0;
        objectDescriptorSets[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        objectDescriptorSets[7].descriptorCount = 1;
        objectDescriptorSets[7].pImageInfo = &textures.emissionMap.imageInfo;

        vkUpdateDescriptorSets(vulkanDevice->logicalDevice, static_cast<uint32_t>(objectDescriptorSets.size()), objectDescriptorSets.data(), 0,
                               nullptr);

        // Skybox
        VK_CHECK_RESULT(vkAllocateDescriptorSets(vulkanDevice->logicalDevice, &allocateInfo, &descriptorSets.skybox));

        std::array<VkWriteDescriptorSet, 3> skyboxDescriptorSets{};
        skyboxDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skyboxDescriptorSets[0].dstSet = descriptorSets.skybox;
        skyboxDescriptorSets[0].dstBinding = 0;
        skyboxDescriptorSets[0].dstArrayElement = 0;
        skyboxDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        skyboxDescriptorSets[0].descriptorCount = 1;
        skyboxDescriptorSets[0].pBufferInfo = &uniformBuffers.skybox.bufferInfo;

        skyboxDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skyboxDescriptorSets[1].dstSet = descriptorSets.skybox;
        skyboxDescriptorSets[1].dstBinding = 1;
        skyboxDescriptorSets[1].dstArrayElement = 0;
        skyboxDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        skyboxDescriptorSets[1].descriptorCount = 1;
        skyboxDescriptorSets[1].pBufferInfo = &uniformBuffers.uboParams.bufferInfo;

        skyboxDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        skyboxDescriptorSets[2].dstSet = descriptorSets.skybox;
        skyboxDescriptorSets[2].dstBinding = 2;
        skyboxDescriptorSets[2].dstArrayElement = 0;
        skyboxDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxDescriptorSets[2].descriptorCount = 1;
        skyboxDescriptorSets[2].pImageInfo = &textures.envMap.imageInfo;

        vkUpdateDescriptorSets(vulkanDevice->logicalDevice, static_cast<uint32_t>(skyboxDescriptorSets.size()), skyboxDescriptorSets.data(), 0, VK_NULL_HANDLE);
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
            vkCmdBindVertexBuffers(drawCommandBuffers[i], 0, 1, &models.helmet.vertexBuffer.buffer, offsets);
            vkCmdBindIndexBuffer(drawCommandBuffers[i], models.helmet.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

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

            if (displaySkybox) {
                vkCmdBindVertexBuffers(drawCommandBuffers[i], 0, 1, &models.envCube.vertexBuffer.buffer, offsets);
                vkCmdBindIndexBuffer(drawCommandBuffers[i], models.envCube.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdBindPipeline(drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.skybox);
                vkCmdBindDescriptorSets(drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.skybox, 0, nullptr);
                vkCmdDrawIndexed(drawCommandBuffers[i], static_cast<uint32_t>(models.envCube.indices.size()), 1, 0, 0, 0);
            }
            vkCmdBindPipeline(drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.pbr);
            vkCmdBindDescriptorSets(drawCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.pbr, 0,nullptr);

            vkCmdDrawIndexed(drawCommandBuffers[i], static_cast<uint32_t>(models.helmet.indices.size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(drawCommandBuffers[i]);
            VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
        }
    }

    void drawFrame() override {
        VulkanApplicationBase::prepareFrame();
        updateUniformBuffers();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &drawCommandBuffers[currentBuffer];
        VK_CHECK_RESULT(vkQueueSubmit(vulkanDevice->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));
        VulkanApplicationBase::submitFrame();
    }

    ~PbrExample() override {
        uniformBuffers.object.cleanUp();
        uniformBuffers.skybox.cleanUp();
        uniformBuffers.uboParams.cleanUp();
        models.helmet.cleanUp();
        models.envCube.cleanUp();
        textures.mainTex.cleanUp();
        textures.roughnessMap.cleanUp();
        textures.occlusionMap.cleanUp();
        textures.normalMap.cleanUp();
        textures.metallicMap.cleanUp();
        textures.emissionMap.cleanUp();
        textures.envMap.cleanUp();

        vkDestroyPipeline(vulkanDevice->logicalDevice, pipelines.pbr, nullptr);
        vkDestroyPipeline(vulkanDevice->logicalDevice, pipelines.skybox, nullptr);
        vkDestroyPipelineLayout(vulkanDevice->logicalDevice, pipelineLayout, nullptr);
        vkDestroyDescriptorPool(vulkanDevice->logicalDevice, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(vulkanDevice->logicalDevice, descriptorSetLayout, nullptr);
    }
};

int main(int argc, char *argv[]) {
    auto *application = new PbrExample();
    application->setupWindow();
    application->initVulkan();
    application->prepare();
    application->renderLoop();
    delete(application);
    return 0;
}
