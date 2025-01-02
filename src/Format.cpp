//
// Created by Sebastian Sandstig on 2024-12-02.
//

#include <Device.h>
#include <Format.h>

namespace Format {
VkFormat find_supported(Device *device, const std::vector<VkFormat> &candidates, VkImageTiling tiling,
                        VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
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
}