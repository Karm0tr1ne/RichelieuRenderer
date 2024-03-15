#ifndef RICHELIEU_VULKANMODEL_H
#define RICHELIEU_VULKANMODEL_H

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <string>
//#include "assimp/Importer.hpp"
//#include "assimp/scene.h"
//#include "assimp/postprocess.h"

#include "VulkanDevice.h"

struct Vertex {
    glm::vec3 pos = glm::vec3();
    glm::vec2 texCoord = glm::vec2();
    glm::vec3 normal = glm::vec3();
    glm::vec4 tangent = glm::vec4();

    static VkVertexInputBindingDescription GetBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions();
};
class Model {
public:
    VulkanBase::VulkanDevice *pDevice;

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
//    void processNode(aiNode *node, const aiScene *scene);
//    void processMesh(aiMesh *mesh, const aiScene *scene);
};
#endif
