//
// Created by Sebastian Sandstig on 2024-11-29.
//

#ifndef IMAGE_H
#define IMAGE_H

#include <vulkan/vulkan.hpp>

struct ImageSpec {
    Device* device;
    uint32_t width;
    uint32_t height;
    VkSampleCountFlagBits num_samples;
    VkFormat format;
    VkImageTiling tiling;
    VkImageUsageFlags usage;
    VkImageAspectFlags aspect_flags;
    VkMemoryPropertyFlags properties;
    std::optional<uint32_t> mip_levels = {};
};


class Image {
public:
    explicit Image(const ImageSpec& spec);
    ~Image();
    void generate_mipmaps(VkCommandPool command_pool_handle) const;
    void transition_layout(VkCommandPool command_pool_handle, VkImageLayout old_layout, VkImageLayout new_layout) const;
    void copy_buffer_to_image(VkCommandPool command_pool_handle, VkBuffer buffer);

    Device* get_device() const;
    uint32_t get_mip_levels() const;

    VkImage get_image() const;
    VkImageView get_image_view() const;
private:
    VkImage m_image_handle;
    VkImageView m_view_handle;
    VkDeviceMemory m_memory;

    Device* m_device = nullptr;
    uint32_t m_width;
    uint32_t m_height;
    VkSampleCountFlagBits m_num_samples;
    VkFormat m_format;
    VkImageTiling m_tiling;
    VkImageUsageFlags m_usage;
    VkImageAspectFlags m_aspect_flags;
    VkMemoryPropertyFlags m_properties;
    uint32_t m_mip_levels;

    static VkImage create_image(VkDevice device_handle, uint32_t width, uint32_t height, uint32_t mip_levels,
                                VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                VkSampleCountFlagBits num_samples);
    static VkImageView create_view(VkDevice device_handle, VkImage handle, VkFormat format,
                                   VkImageAspectFlags aspect_flags, uint32_t mip_levels);
    static VkDeviceMemory bind_image_memory(Device* device, VkImage handle, VkMemoryPropertyFlags properties);
    static bool has_stencil_component(VkFormat format);
};


#endif //IMAGE_H
