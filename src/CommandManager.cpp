//
// Created by Sebastian Sandstig on 2024-11-29.
//

#include "CommandManager.h"

namespace command {

CommandManager::CommandManager(PoolSpec &pool_spec) {
    m_pool_spec = pool_spec;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_pool_spec.queues.graphicsFamily.value();

    if (vkCreateCommandPool(m_pool_spec.device, &poolInfo, nullptr, &m_command_pool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

std::vector<size_t> CommandManager::get_unique_descriptors(size_t count) {
    std::vector<size_t> unique_indices;

    while (!m_empty_indices.empty() && unique_indices.size() < count) {
        size_t index = *m_empty_indices.begin();
        unique_indices.push_back(index);
        m_empty_indices.erase(index);
    }
    m_command_buffers.resize(m_command_buffers.size() + count - unique_indices.size());


    std::vector<VkCommandBuffer> new_commands;
    new_commands.resize(count);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(count);

    if (vkAllocateCommandBuffers(m_pool_spec.device, &allocInfo, new_commands.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < count; i++) {
        m_command_buffers[unique_indices[i]] = new_commands[i];
    }

    // commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //
    // VkCommandBufferAllocateInfo allocInfo{};
    // allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // allocInfo.commandPool = commandPool;
    // allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();
    //
    // if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    //     throw std::runtime_error("failed to allocate command buffers!");
    // }
}



} // command