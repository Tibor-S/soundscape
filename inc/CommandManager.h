//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H
#include <set>
#include <vulkan/vulkan.hpp>

#include "Queue.h"

namespace command {

struct PoolSpec {
    VkDevice device;
    Queue::FamilyIndices queues;
};

class CommandManager {
public:
explicit CommandManager(PoolSpec &pool_spec);
std::vector<size_t> get_unique_descriptors(size_t count);

private:
    VkCommandPool m_command_pool {};
    std::vector<VkCommandBuffer> m_command_buffers {};
    std::set<size_t> m_empty_indices;

    PoolSpec m_pool_spec {};
};

} // command

#endif //COMMANDMANAGER_H
