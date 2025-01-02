//
// Created by Sebastian Sandstig on 2024-12-01.
//

#include <SwapChain.h>

#include <Format.h>

SwapChain::SwapChain(const SwapChainSpec& spec) : DeviceParent(spec.device) {
    m_window = spec.window;
    m_surface_handle = spec.surface_handle;
    auto swap_chain_support = spec.device->swap_chain_support(m_surface_handle);
    m_surface_format = choose_swap_surface_format(swap_chain_support.formats);
    m_extent = choose_swap_extent(swap_chain_support.capabilities, m_window);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.presentModes);
    m_swap_chain_handle = create_swap_chain(get_device(), spec.surface_handle, m_window, &swap_chain_support,
                                    &m_surface_format, &m_extent, present_mode, spec.max_image_count);
    m_images = create_images(get_device()->logical_device_handle(), m_swap_chain_handle);
    m_image_views = create_image_views(get_device()->logical_device_handle(), m_images, m_surface_format.format);
    m_color_image = create_color_image(get_device(), m_surface_format, m_extent);
    m_depth_image = create_depth_image(get_device(), find_depth_format(), m_extent);
}

SwapChain::~SwapChain() {
    delete m_color_image;
    delete m_depth_image;

    for (const auto & m_image_view : m_image_views) {
        vkDestroyImageView(get_device()->logical_device_handle(), m_image_view, nullptr);
    }

    vkDestroySwapchainKHR(get_device()->logical_device_handle(), m_swap_chain_handle, nullptr);

}


VkSwapchainKHR SwapChain::create_swap_chain(const Device *device, VkSurfaceKHR surface_handle, GLFWwindow *window,
                                            const SwapChainSupportDetails *swap_chain_support,
                                            VkSurfaceFormatKHR *surface_format, const VkExtent2D *extent,
                                            const VkPresentModeKHR present_mode, const size_t max_image_count)
{
    uint32_t image_count = swap_chain_support->capabilities.minImageCount + 1;
    if (swap_chain_support->capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support->capabilities.maxImageCount)
    {
        image_count = swap_chain_support->capabilities.maxImageCount;
    }
    image_count = std::min(image_count, static_cast<uint32_t>(max_image_count));

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_handle;

    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format->format;
    create_info.imageColorSpace = surface_format->colorSpace;
    create_info.imageExtent = *extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const uint32_t graphics_queue = device->queue_index(GRAPHICS).value();
    const uint32_t presentation_queue = device->queue_index(PRESENTATION).value();
    const uint32_t queueFamilyIndices[] = {
        graphics_queue,
        presentation_queue};

    if (graphics_queue != presentation_queue) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0; // Optional
        create_info.pQueueFamilyIndices = nullptr; // Optional
    }

    create_info.preTransform = swap_chain_support->capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swap_chain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(device->logical_device_handle(), &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    return swap_chain;
}

std::vector<VkImageView> SwapChain::create_image_views(VkDevice device_handle, const std::vector<VkImage> &images,
                                                       const VkFormat format) {
    std::vector<VkImageView> image_views;
    image_views.resize(images.size());

    for (uint32_t i = 0; i < images.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device_handle, &viewInfo, nullptr, &image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
    }

    return image_views;
}

std::vector<VkImage> SwapChain::create_images(VkDevice device_handle, VkSwapchainKHR swap_chain_handle) {
    uint32_t image_count = 0;
    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(device_handle, swap_chain_handle, &image_count, nullptr);
    images.resize(image_count);
    vkGetSwapchainImagesKHR(device_handle, swap_chain_handle, &image_count, images.data());
    return images;
}

VkSurfaceFormatKHR SwapChain::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace ==
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR SwapChain::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

Image *SwapChain::create_color_image(Device *device, const VkSurfaceFormatKHR surface_format,
                                     const VkExtent2D extent) {
    ImageSpec spec = {};
    spec.device = device;
    spec.width = extent.width;
    spec.height = extent.height;
    spec.mip_levels = 1;
    spec.num_samples = device->max_sample_count();
    spec.format = surface_format.format;
    spec.tiling = VK_IMAGE_TILING_OPTIMAL;
    spec.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    spec.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    spec.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    return new Image(spec);
}

VkFormat SwapChain::find_depth_format() const {
    return find_supported_format(
        get_device(),
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

Image* SwapChain::create_depth_image(Device* device, const VkFormat depth_format, const VkExtent2D extent) {
    ImageSpec spec = {};
    spec.device = device;
    spec.width = extent.width;
    spec.height = extent.height;
    spec.mip_levels = 1;
    spec.num_samples = device->max_sample_count();
    spec.format = depth_format;
    spec.tiling = VK_IMAGE_TILING_OPTIMAL;
    spec.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    spec.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    spec.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    return new Image(spec);
}

