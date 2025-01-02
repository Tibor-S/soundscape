//
// Created by Sebastian Sandstig on 2024-12-02.
//

#ifndef FORMAT_H
#define FORMAT_H

#include <vulkan/vulkan.hpp>

VkFormat find_supported_format(const Device *device, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                               VkFormatFeatureFlags features);
#endif //FORMAT_H
