//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef TEXTUREIMAGE_H
#define TEXTUREIMAGE_H
#include <SamplerImage.h>

namespace TextureImage {
class LoadImage {
public:
    explicit LoadImage(const char* path) {
        m_pixels = stbi_load(path, &m_tex_width, &m_tex_height, &m_tex_channels, STBI_rgb_alpha);
        m_image_size = m_tex_width * m_tex_height * 4;

        if (!m_pixels) {
            throw std::runtime_error("failed to load texture image!");
        }
    }
    ~LoadImage() {
        stbi_image_free(m_pixels);
    }

    [[nodiscard]] int get_width() const { return m_tex_width; }
    [[nodiscard]] int get_height() const { return m_tex_height; }
    [[nodiscard]] int get_channels() const { return m_tex_channels; }
    [[nodiscard]] size_t get_image_size() const { return m_image_size; }
    [[nodiscard]] stbi_uc* get_pixels() const { return m_pixels; }
private:
    int m_tex_width = 0, m_tex_height = 0, m_tex_channels = 0;
    size_t m_image_size;
    stbi_uc *m_pixels;
};

struct Spec {
    Device* device;
    LoadImage* src_image;
    VkCommandPool command_pool;
    VkImageTiling tiling;
    std::array<VkSamplerAddressMode, 3> address_modes;
    VkBorderColor border_color;
    VkBool32 compare_enable;
    VkCompareOp compare_op;
};

class TextureImage : public SamplerImage {
public:
    explicit TextureImage(Spec& spec);
    ~TextureImage();
private:
    static SamplerImageSpec get_sampler_spec(Spec& spec) {
        SamplerImageSpec sampler_image_spec = {};
        sampler_image_spec.device = spec.device;
        sampler_image_spec.width = spec.src_image->get_width();
        sampler_image_spec.height = spec.src_image->get_height();
        sampler_image_spec.num_samples = VK_SAMPLE_COUNT_1_BIT;
        sampler_image_spec.format = VK_FORMAT_R8G8B8A8_SRGB;
        sampler_image_spec.tiling = spec.tiling;
        sampler_image_spec.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        sampler_image_spec.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
        sampler_image_spec.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        sampler_image_spec.min_filter = VK_FILTER_LINEAR;
        sampler_image_spec.mag_filter = VK_FILTER_LINEAR;
        sampler_image_spec.address_modes = spec.address_modes;
        sampler_image_spec.border_color = spec.border_color;
        sampler_image_spec.un_normalized = VK_FALSE;
        sampler_image_spec.compare_enable = spec.compare_enable;
        sampler_image_spec.compare_op = spec.compare_op;
        sampler_image_spec.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_image_spec.min_lod = 0.0f;
        sampler_image_spec.mip_lod_bias = 0.0f;
        return sampler_image_spec;
    };
};

class TextureImageParent {
public:
    explicit TextureImageParent(TextureImage* texture) {m_texture = texture;}
    [[nodiscard]] TextureImage* get_texture() const {return m_texture;}
private:
    TextureImage* m_texture;
};
}


#endif //TEXTUREIMAGE_H
