//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef QUEUE_H
#define QUEUE_H

#include <vulkan/vulkan.hpp>

enum QueueType {
    GRAPHICS,
    PRESENTATION
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool is_complete(bool uses_presentation) {
        return graphicsFamily.has_value() && (
            (uses_presentation && presentFamily.has_value()) ||
            !uses_presentation);
    }
};

QueueFamilyIndices find_families(VkPhysicalDevice physical_device, std::optional<VkSurfaceKHR> surface);

#endif //QUEUE_H
