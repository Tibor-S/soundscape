//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef DEVICE_H
#define DEVICE_H
#include <map>
#include <Queue.h>
#include <SwapChainSupport.h>
#include <vulkan/vulkan.hpp>

namespace Device {

struct Spec {
    VkInstance instance;
};

class Device {
public:
    explicit Device(Spec& spec, std::optional<VkSurfaceKHR> uses_surface = {});
    VkDevice logical_device_handle() const;
    VkPhysicalDevice physical_device_handle() const;
    std::optional<size_t> queue_index(Queue::Type queue_type) const;
    std::optional<VkQueue> queue(Queue::Type queue_type) const;
    VkSampleCountFlagBits max_sample_count() const;

    SwapChainSupport::SwapChainSupportDetails get_swap_chain_support(VkSurfaceKHR surface) const;

    void destroy() const;

private:
    Spec m_spec{};
    VkPhysicalDevice m_physical_device;
    VkSampleCountFlagBits m_msaa_samples;
    Queue::FamilyIndices m_queue_families;
    VkDevice m_logical_device;
    std::map<Queue::Type, VkQueue> m_queue;
    SwapChainSupport::SwapChainSupportDetails m_swap_chain_support;


    static VkPhysicalDevice pick_physical_device(VkInstance instance, std::optional<VkSurfaceKHR> uses_surface);
    static bool is_device_suitable(VkPhysicalDevice physical_device, std::optional<VkSurfaceKHR> uses_surface);
    static bool check_device_extension_support(VkPhysicalDevice physical_device);
    static SwapChainSupport::SwapChainSupportDetails query_swap_chain_support(
            VkPhysicalDevice physical_device, VkSurfaceKHR surface);
    static VkSampleCountFlagBits max_usable_sample_count(VkPhysicalDevice physical_device);
    static VkDevice create_logical_device(VkPhysicalDevice physical_device, Queue::FamilyIndices queue_indices);
};

class DeviceParent {
public:
    explicit DeviceParent(Device* device) {m_device = device;}
    Device* get_device() const {return m_device;}
private:
    Device* m_device;
};

} // Device

#endif //DEVICE_H
