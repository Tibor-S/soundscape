//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef BUFFER_H
#define BUFFER_H
#include <Device.h>
#include <vulkan/vulkan.hpp>


namespace Buffer {

VkBuffer create_buffer(Device* device, VkDeviceSize size, VkBufferUsageFlags usage);
VkDeviceMemory bind_buffer(Device* device, VkBuffer buffer_handle, VkMemoryPropertyFlags properties);
void copy_buffer(Device *device, VkCommandPool command_pool, VkBuffer src_buffer, VkBuffer dst_buffer,
                 VkDeviceSize size);
} // Buffer

#endif //BUFFER_H
