#ifndef RICHELIEU_VULKANMODEL_H
#define RICHELIEU_VULKANMODEL_H

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <string>
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "VulkanDevice.h"

struct Vertex {
    glm::vec3 pos = glm::vec3();
    glm::vec3 color = glm::vec3();
    glm::vec2 texCoord = glm::vec2();
    glm::vec3 normal = glm::vec3();
    glm::vec4 tangent = glm::vec4();

    static VkVertexInputBindingDescription GetBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 5> GetAttributeDescriptions(){
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

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

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, normal);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, tangent);

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
    void loadFromObj(std::string filePath, VulkanBase::VulkanDevice *device);
    void cleanUp();

private:
    void createBuffer();
    void processNode(aiNode *node, const aiScene *scene);
    void processMesh(aiMesh *mesh, const aiScene *scene);
};
#endif
