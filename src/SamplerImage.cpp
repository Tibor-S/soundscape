//
// Created by Sebastian Sandstig on 2024-12-01.
//

#include <SamplerImage.h>

SamplerImage::SamplerImage(const SamplerImageSpec& spec) : Image::Image(spec){
    m_min_filter = spec.min_filter;
    m_mag_filter = spec.mag_filter;
    m_address_modes[0] = spec.address_modes[0];
    m_address_modes[1] = spec.address_modes[1];
    m_address_modes[2] = spec.address_modes[2];
    m_border_color = spec.border_color;
    m_un_normalized = spec.un_normalized;
    m_compare_enable = spec.compare_enable;
    m_compare_op = spec.compare_op;
    m_mipmap_mode = spec.mipmap_mode;
    m_min_lod = spec.min_lod;
    m_mip_lod_bias = spec.mip_lod_bias;
    m_sampler_handle = create_sampler(get_device(), m_mag_filter, m_min_filter, m_address_modes, m_border_color, m_un_normalized, m_compare_enable, m_compare_op,  m_mipmap_mode, m_min_lod, m_mip_lod_bias, get_mip_levels());
}

SamplerImage::~SamplerImage() {
    vkDestroySampler(get_device()->logical_device_handle(), m_sampler_handle, nullptr);
}


VkSampler SamplerImage::get_sampler() const {
    return m_sampler_handle;
}


// Private

VkSampler SamplerImage::create_sampler(Device* device, VkFilter mag_filter, VkFilter min_filter,
                                  std::array<VkSamplerAddressMode, 3> &address_modes, VkBorderColor border_color,
                                  VkBool32 un_normalized, VkBool32 compare_enable, VkCompareOp compare_op,
                                  VkSamplerMipmapMode mipmap_mode, float min_lod, float mip_lod_bias,
                                  uint32_t mip_levels) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device->physical_device_handle(), &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mip_levels);
    samplerInfo.mipLodBias = 0.0f;

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(device->logical_device_handle(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    return sampler;
}
