//
// Created by Sebastian Sandstig on 2024-12-02.
//

#include <Device.h>
#include <Format.h>

VkFormat find_supported_format(const Device *device, const std::vector<VkFormat> &candidates,
                               const VkImageTiling tiling, const VkFormatFeatureFlags features) {
    for (const VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(device->physical_device_handle(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}
