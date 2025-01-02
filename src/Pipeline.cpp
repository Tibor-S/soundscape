//
// Created by Sebastian Sandstig on 2024-12-03.
//

#include <Pipeline.h>

#include <fstream>
#include <iostream>

VkShaderModule create_shader_module(const Device* device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shader_module;
    if (vkCreateShaderModule(device->logical_device_handle(), &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shader_module;
}

std::vector<char> read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t file_size = (size_t) file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), static_cast<int>(file_size));
    file.close();

    return buffer;
}