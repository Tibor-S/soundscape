//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef QUEUE_H
#define QUEUE_H
#include <vulkan/vulkan.hpp>

namespace Queue {

enum Type {
    GRAPHICS,
    PRESENTATION
};

struct FamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool is_complete(bool uses_presentation) {
        return graphicsFamily.has_value() && (
            (uses_presentation && presentFamily.has_value()) ||
            !uses_presentation);
    }
};

FamilyIndices find_families(VkPhysicalDevice physical_device, std::optional<VkSurfaceKHR> surface);

}
#endif //QUEUE_H
