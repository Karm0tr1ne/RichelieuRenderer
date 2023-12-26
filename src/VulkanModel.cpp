#include "VulkanModel.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <stdexcept>
#include <iostream>
#include <array>

void Model::loadFromFile(std::string filePath, VulkanBase::VulkanDevice *device) {
    this->pDevice = device;

    Assimp::Importer importer;
    // TODO: maybe need to flip Y axis
    const aiScene *scene = importer.ReadFile(filePath, aiProcess_Triangulate);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "failed to load model" << importer.GetErrorString() << std::endl;
        return;
    }
    processNode(scene->mRootNode, scene);
    std::cout << "Loading New Model: " << filePath << std::endl;
    // std::cout << " Model: " << shape.name << std::endl;
    std::cout << " Vertices: " << vertices.size() << std::endl;

    createBuffer();
}


void Model::cleanUp() {
    vkDestroyBuffer(pDevice->logicalDevice, vertexBuffer.buffer, nullptr);
    vkFreeMemory(pDevice->logicalDevice, vertexBuffer.memory, nullptr);
    vkDestroyBuffer(pDevice->logicalDevice, indexBuffer.buffer, nullptr);
    vkFreeMemory(pDevice->logicalDevice, indexBuffer.memory, nullptr);
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    for (uint32_t i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }
    for(uint32_t i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

void Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};
        glm::vec3 temp3;
        temp3.x = mesh->mVertices[i].x;
        temp3.y = mesh->mVertices[i].y;
        temp3.z = mesh->mVertices[i].z;
        vertex.pos = temp3;
        if (mesh->HasNormals()) {
            temp3.x = mesh->mNormals[i].x;
            temp3.y = mesh->mNormals[i].y;
            temp3.z = mesh->mNormals[i].z;
            vertex.normal = temp3;
        }
        if (mesh->mTextureCoords[0]) {
            glm::vec2 temp2;
            temp2.x = mesh->mTextureCoords[0][i].x;
            temp2.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoord = temp2;
            glm::vec4 temp4;
            temp4.x = mesh->mTangents[i].x;
            temp4.y = mesh->mTangents[i].y;
            temp4.z = mesh->mTangents[i].z;
            temp4.w = 0;
            vertex.tangent = temp4;
        }
        else {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }
    for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (uint32_t j = 0; i < face.mNumIndices; i++) {
            indices.push_back(face.mIndices[j]);
        }
    }
}

void Model::loadFromObj(std::string filePath, VulkanBase::VulkanDevice *device) {
    this->pDevice = device;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string error;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &error, filePath.c_str())) {
        throw std::runtime_error("failed to load model: " + error);
    }

    for (const auto &shape: shapes) {
        for (const auto &index: shape.mesh.indices) {
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
    }
    createBuffer();
}

void Model::createBuffer() {
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

    VkCommandBuffer copyCmd = pDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkBufferCopy copyRegion{};

    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, vertexStaging.buffer, vertexBuffer.buffer, 1, &copyRegion);

    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(copyCmd, indexStaging.buffer, indexBuffer.buffer, 1, &copyRegion);

    pDevice->flushCommandBuffer(copyCmd, pDevice->graphicsQueue, true);

    vkDestroyBuffer(pDevice->logicalDevice, vertexStaging.buffer, nullptr);
    vkFreeMemory(pDevice->logicalDevice, vertexStaging.memory, nullptr);
    vkDestroyBuffer(pDevice->logicalDevice, indexStaging.buffer, nullptr);
    vkFreeMemory(pDevice->logicalDevice, indexStaging.memory, nullptr);
}

std::array<VkVertexInputAttributeDescription, 5> Vertex::GetAttributeDescriptions() {
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

VkVertexInputBindingDescription Vertex::GetBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}
