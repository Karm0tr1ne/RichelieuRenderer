#ifndef RICHELIEU_VULKANMODEL_H
#define RICHELIEU_VULKANMODEL_H

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <string>

#include "VulkanDevice.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions(){
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }
};
class Model {
public:
    VulkanBase::VulkanDevice *pDevice;
    VkDescriptorPool descriptorPool;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string path;

    struct {
        int count;
        VkBuffer buffer;
        VkDeviceMemory memory;
    } vertexBuffer;

    struct {
        int count;
        VkBuffer buffer;
        VkDeviceMemory memory;
    } indexBuffer;

    void loadFromFile(std::string filePath, VulkanBase::VulkanDevice *device);
    void cleanUp();
};
#endif