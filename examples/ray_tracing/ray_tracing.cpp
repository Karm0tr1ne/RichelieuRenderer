#include "VulkanRayTracingApplicationBase.h"

class RayTracing : public VulkanBase::VulkanRayTracingApplicationBase {
public:
    struct ShaderBindingTables {
        VulkanBase::ShaderBindingTable raygen;
        VulkanBase::ShaderBindingTable miss;
        VulkanBase::ShaderBindingTable hit;
    } shaderBindingTables;

    struct UniformData {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
        glm::vec4 lightPos;
        int32_t vertexSize;
    } uniformData;
    VulkanBase::VulkanBuffer ubo;

    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout descriptorSetLayout;

    RayTracing() : VulkanBase::VulkanRayTracingApplicationBase() {
        title = "Ray Tracing Example";
        enableExtensions();
    }

    ~RayTracing() {
        vkDestroyPipeline(vulkanDevice->logicalDevice, pipeline, nullptr);
        vkDestroyPipelineLayout(vulkanDevice->logicalDevice, pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(vulkanDevice->logicalDevice, descriptorSetLayout, nullptr);
        cleanUpStorageImage();
        bottomLevelAS.cleanUp();
        topLevelAS.cleanUp();
        shaderBindingTables.raygen.cleanUp();
        shaderBindingTables.hit.cleanUp();
        shaderBindingTables.miss.cleanUp();
        ubo.cleanUp();
    }
};

int main(int argc, char *argv[]) {
    auto *application = new RayTracing();
    application->setupWindow();
    application->initVulkan();
    application->prepare();
    application->renderLoop();
    delete(application);
    return 0;
}