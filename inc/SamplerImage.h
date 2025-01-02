//
// Created by Sebastian Sandstig on 2024-12-01.
//

#ifndef SAMPLER_H
#define SAMPLER_H

#include <vulkan/vulkan.hpp>

#include <Device.h>
#include <Image.h>

struct SamplerImageSpec : ImageSpec {
    VkFilter min_filter;
    VkFilter mag_filter;
    std::array<VkSamplerAddressMode, 3> address_modes;
    VkBorderColor border_color;
    VkBool32 un_normalized;
    VkBool32 compare_enable;
    VkCompareOp compare_op;
    VkSamplerMipmapMode mipmap_mode;
    float min_lod;
    float mip_lod_bias;
};

class SamplerImage : public Image {
public:
    explicit SamplerImage(const SamplerImageSpec &spec);
    ~SamplerImage();
    [[nodiscard]] VkSampler get_sampler() const;

private:
    VkSampler m_sampler_handle;

    VkFilter m_min_filter;
    VkFilter m_mag_filter;
    std::array<VkSamplerAddressMode, 3> m_address_modes {};
    VkBorderColor m_border_color;
    VkBool32 m_un_normalized;
    VkBool32 m_compare_enable;
    VkCompareOp m_compare_op;
    VkSamplerMipmapMode m_mipmap_mode;
    float m_min_lod;
    float m_mip_lod_bias;

    static VkSampler create_sampler(Device *device, VkFilter mag_filter, VkFilter min_filter,
                                    std::array<VkSamplerAddressMode, 3> &address_modes, VkBorderColor border_color,
                                    VkBool32 un_normalized, VkBool32 compare_enable, VkCompareOp compare_op,
                                    VkSamplerMipmapMode mipmap_mode, float min_lod, float mip_lod_bias,
                                    uint32_t mip_levels);
};

#endif //SAMPLER_H
