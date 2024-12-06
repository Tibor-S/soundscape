//
// Created by Sebastian Sandstig on 2024-12-02.
//

#include <Command.h>

namespace Command {
VkCommandBuffer begin_single_time_command(Device::Device* device, VkCommandPool command_pool_handle) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = command_pool_handle;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device->logical_device_handle(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void end_single_time_command(Device::Device *device, VkCommandPool command_pool_handle,
                             VkCommandBuffer command_buffer_handle) {
    vkEndCommandBuffer(command_buffer_handle);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer_handle;

    // VkQueue
    VkQueue graphics_queue = device->queue(Queue::GRAPHICS).value();
    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device->logical_device_handle(), command_pool_handle, 1, &command_buffer_handle);

}


}