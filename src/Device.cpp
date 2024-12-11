//
// Created by Sebastian Sandstig on 2024-11-29.
//

#include <Device.h>
#include <HelloTriangleApplication.h>

#include <Queue.h>
#include <set>
#include <SwapChainSupport.h>

namespace Device {
Device::Device(Spec& spec, std::optional<VkSurfaceKHR> uses_surface) {
    m_spec = spec;
    m_physical_device = pick_physical_device(m_spec.instance, uses_surface);
    m_msaa_samples = max_usable_sample_count(m_physical_device);
    m_queue_families = Queue::find_families(m_physical_device, uses_surface);
    m_logical_device = create_logical_device(m_physical_device, m_queue_families);
    m_queue[Queue::Type::GRAPHICS] = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_logical_device, m_queue_families.graphicsFamily.value(), 0, &m_queue[Queue::Type::GRAPHICS]);
    if (uses_surface.has_value() && m_queue_families.presentFamily.has_value()) {
        m_queue[Queue::Type::PRESENTATION] = VK_NULL_HANDLE;
        vkGetDeviceQueue(m_logical_device, m_queue_families.presentFamily.value(), 0, &m_queue[Queue::Type::PRESENTATION]);
    }
}

VkDevice Device::logical_device_handle() const {
    return m_logical_device;
}

VkPhysicalDevice Device::physical_device_handle() const {
    return m_physical_device;
}

std::optional<size_t> Device::queue_index(Queue::Type queue_type) const {
    switch (queue_type) {
        case Queue::Type::GRAPHICS:
            return m_queue_families.graphicsFamily;
        case Queue::Type::PRESENTATION:
            return m_queue_families.presentFamily;
        default:
            return {};
    }
}

std::optional<VkQueue> Device::queue(Queue::Type queue_type) const {
    if (auto queue = m_queue.find(queue_type); queue != m_queue.end())
        return queue->second;
    else
        return {};
}


VkSampleCountFlagBits Device::max_sample_count() const {
    return m_msaa_samples;
}

SwapChainSupport::SwapChainSupportDetails Device::get_swap_chain_support(VkSurfaceKHR surface) const {
    return Device::query_swap_chain_support(m_physical_device, surface);
}
void Device::destroy() const {
    vkDestroyDevice(m_logical_device, nullptr);
}

// Private
VkPhysicalDevice Device::pick_physical_device(VkInstance instance, std::optional<VkSurfaceKHR> uses_surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& physical_device : devices) {
        if (is_device_suitable(physical_device, uses_surface)) {
            return physical_device;
            // msaaSamples = getMaxUsableSampleCount();
            // break;
        }
    }

    throw std::runtime_error("failed to find a suitable GPU!");
}

bool Device::is_device_suitable(VkPhysicalDevice physical_device, std::optional<VkSurfaceKHR> uses_surface) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(physical_device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physical_device, &deviceFeatures);

    Queue::FamilyIndices indices = Queue::find_families(physical_device, uses_surface);
    if (!indices.is_complete(uses_surface.has_value())) return false;

    bool extensions_supported = check_device_extension_support(physical_device);
    if (!extensions_supported) return false;


    if (uses_surface.has_value()) {
        auto [_,
            formats,
            presentModes] = query_swap_chain_support(physical_device, uses_surface.value());
        if (formats.empty() || presentModes.empty())
            return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physical_device, &supportedFeatures);
    if (!supportedFeatures.samplerAnisotropy) return false;

    return true;
}

bool Device::check_device_extension_support(VkPhysicalDevice physical_device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

SwapChainSupport::SwapChainSupportDetails Device::query_swap_chain_support(
    VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    SwapChainSupport::SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSampleCountFlagBits Device::max_usable_sample_count(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts & physical_device_properties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

VkDevice Device::create_logical_device(VkPhysicalDevice physical_device, Queue::FamilyIndices queue_indices) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        queue_indices.graphicsFamily.value(), queue_indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE; // enable sample shading feature for the device

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enable_validation_layers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        createInfo.ppEnabledLayerNames = validation_layers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    VkDevice logical_device;
    if (vkCreateDevice(physical_device, &createInfo, nullptr, &logical_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
    return logical_device;
}


} // Device