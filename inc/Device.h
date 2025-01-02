//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef DEVICE_H
#define DEVICE_H

#include <map>

#include <vulkan/vulkan.hpp>

#include <Queue.h>
#include <SwapChainSupport.h>

struct DeviceSpec {
    VkInstance instance;
};

class Device {
public:
    explicit Device(const DeviceSpec& spec, std::optional<VkSurfaceKHR> uses_surface = {});
    [[nodiscard]] VkDevice logical_device_handle() const;
    [[nodiscard]] VkPhysicalDevice physical_device_handle() const;
    [[nodiscard]] std::optional<size_t> queue_index(QueueType queue_type) const;
    [[nodiscard]] std::optional<VkQueue> queue(QueueType queue_type) const;
    [[nodiscard]] VkSampleCountFlagBits max_sample_count() const;

    [[nodiscard]] SwapChainSupportDetails swap_chain_support(VkSurfaceKHR surface) const;

    void destroy() const;

private:
    DeviceSpec m_spec{};
    VkPhysicalDevice m_physical_device;
    VkSampleCountFlagBits m_msaa_samples;
    QueueFamilyIndices m_queue_families;
    VkDevice m_logical_device;
    std::map<QueueType, VkQueue> m_queue;
    SwapChainSupportDetails m_swap_chain_support {};


    static VkPhysicalDevice pick_physical_device(VkInstance instance, std::optional<VkSurfaceKHR> uses_surface);
    static bool is_device_suitable(VkPhysicalDevice physical_device, std::optional<VkSurfaceKHR> uses_surface);
    static bool check_device_extension_support(VkPhysicalDevice physical_device);
    static SwapChainSupportDetails query_swap_chain_support(
            VkPhysicalDevice physical_device, VkSurfaceKHR surface);
    static VkSampleCountFlagBits max_usable_sample_count(VkPhysicalDevice physical_device);
    static VkDevice create_logical_device(VkPhysicalDevice physical_device, QueueFamilyIndices queue_indices);
};

class DeviceParent {
public:
    explicit DeviceParent(Device* device) {m_device = device;}
    [[nodiscard]] Device* get_device() const {return m_device;}
private:
    Device* m_device;
};

#endif //DEVICE_H
