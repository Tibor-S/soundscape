//
// Created by Sebastian Sandstig on 2024-12-02.
//

#ifndef COMMAND_H
#define COMMAND_H

#include <Device.h>
#include <vulkan/vulkan.hpp>

namespace Command {
VkCommandBuffer begin_single_time_command(Device::Device* device, VkCommandPool command_pool_handle);
void end_single_time_command(Device::Device *device, VkCommandPool command_pool_handle,
                             VkCommandBuffer command_buffer_handle);
}

#endif //COMMAND_H
