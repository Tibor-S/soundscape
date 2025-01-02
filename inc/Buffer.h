//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef BUFFER_H
#define BUFFER_H
#include <Device.h>
#include <vulkan/vulkan.hpp>


class Buffer {
public:
    static VkBuffer create_buffer(const Device* device, VkDeviceSize size, VkBufferUsageFlags usage);
    static VkDeviceMemory bind_buffer(const Device* device, VkBuffer buffer_handle, VkMemoryPropertyFlags properties);
    static void copy_buffer(Device *device, VkCommandPool command_pool, VkBuffer src_buffer, VkBuffer dst_buffer,
                            VkDeviceSize size);
};

#endif //BUFFER_H
