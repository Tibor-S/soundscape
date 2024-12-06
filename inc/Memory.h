//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef MEMORY_H
#define MEMORY_H
#include <vulkan/vulkan.hpp>

namespace Memory {
size_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
}

#endif //MEMORY_H
