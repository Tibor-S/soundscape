//
// Created by Sebastian Sandstig on 2024-11-29.
//

#include <Queue.h>

namespace Queue {

FamilyIndices find_families(VkPhysicalDevice physical_device, std::optional<VkSurfaceKHR> surface) {
    Queue::FamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queueFamily : queue_families) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (surface.has_value()) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface.value(), &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }

        if (indices.is_complete(surface.has_value())) {
            break;
        }

        i++;
    }

    return indices;
}


}
