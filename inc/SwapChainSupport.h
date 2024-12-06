//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef SWAPCHAINSUPPORT_H
#define SWAPCHAINSUPPORT_H
#include <vector>
#include <vulkan/vulkan.hpp>

namespace SwapChainSupport {
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
}

#endif //SWAPCHAINSUPPORT_H
