//
// Created by Sebastian Sandstig on 2024-12-04.
//

#include <Buffer.h>
#include <Command.h>
#include <Memory.h>

namespace Buffer {

VkBuffer create_buffer(Device::Device* device, VkDeviceSize size, VkBufferUsageFlags usage) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer_handle;
    if (vkCreateBuffer(device->logical_device_handle(), &bufferInfo, nullptr, &buffer_handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    return buffer_handle;

}

VkDeviceMemory bind_buffer(Device::Device* device, VkBuffer buffer_handle, VkMemoryPropertyFlags properties) {
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device->logical_device_handle(), buffer_handle, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Memory::find_memory_type(device->physical_device_handle(),
                                                         memRequirements.memoryTypeBits, properties);

    VkDeviceMemory memory_handle;
    if (vkAllocateMemory(device->logical_device_handle(), &allocInfo, nullptr, &memory_handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device->logical_device_handle(), buffer_handle, memory_handle, 0);

    return memory_handle;
}

void copy_buffer(Device::Device *device, VkCommandPool command_pool, VkBuffer src_buffer, VkBuffer dst_buffer,
                 VkDeviceSize size) {
    VkCommandBuffer command_buffer = Command::begin_single_time_command(device, command_pool);

    VkBufferCopy copy_region{};
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

    Command::end_single_time_command(device, command_pool, command_buffer);
}

}
