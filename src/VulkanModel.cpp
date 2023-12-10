#include "VulkanModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>

void Model::loadFromFile(std::string filePath, VulkanBase::VulkanDevice *device) {
    this->pDevice = device;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &error, filePath.c_str())) {
        throw std::runtime_error("failed to load model: " + error);
    }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2],
            };
            vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            vertex.color = {1.0f, 1.0f, 1.0f};
            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
        std::cout << "Loading New Model: " << filePath << std::endl;
        std::cout << " Model: " << shape.name << std::endl;
        std::cout << " Vertices: " << vertices.size() << std::endl;

        size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
        size_t indexBufferSize = indices.size() * sizeof(uint32_t);
        indexBuffer.count = static_cast<uint32_t>(indices.size());
        vertexBuffer.count = static_cast<uint32_t>(vertices.size());

        struct StagingBuffer {
            VkBuffer buffer;
            VkDeviceMemory memory;
        } vertexStaging, indexStaging;

        pDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &vertexStaging.buffer,
                              &vertexStaging.memory,
                              vertexBufferSize,
                              vertices.data());
        pDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &indexStaging.buffer,
                              &indexStaging.memory,
                              indexBufferSize,
                              indices.data());
        pDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             &vertexBuffer.buffer,
                             &vertexBuffer.memory,
                              vertexBufferSize);
        pDevice->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                              &indexBuffer.buffer,
                              &indexBuffer.memory,
                              indexBufferSize);

        VkCommandBuffer copyCmd = device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBufferCopy copyRegion{};

        copyRegion.size = vertexBufferSize;
        vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, 1, &copyRegion);

        copyRegion.size = indexBufferSize;
        vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, 1, &copyRegion);

        device->flushCommandBuffer(copyCmd, pDevice->graphicsQueue, true);

        vkDestroyBuffer(device->logicalDevice, vertexStaging.buffer, nullptr);
        vkFreeMemory(device->logicalDevice, vertexStaging.memory, nullptr);
        vkDestroyBuffer(device->logicalDevice, indexStaging.buffer, nullptr);
        vkFreeMemory(device->logicalDevice, indexStaging.memory, nullptr);
    }
}

void Model::cleanUp() {
    vkDestroyBuffer(pDevice->logicalDevice, vertexBuffer.buffer, nullptr);
    vkFreeMemory(pDevice->logicalDevice, vertexBuffer.memory, nullptr);
    vkDestroyBuffer(pDevice->logicalDevice, indexBuffer.buffer, nullptr);
    vkFreeMemory(pDevice->logicalDevice, indexBuffer.memory, nullptr);
}
