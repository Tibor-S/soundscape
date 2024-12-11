//
// Created by Sebastian Sandstig on 2024-12-06.
//

#ifndef TEXTURE_H
#define TEXTURE_H
#include <Globals.h>
#include <SamplerImage.h>
#include <StagingBuffer.h>
#include <stb_image.h>

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

class Texture : public LoadImage {
public:
    enum Kind {
        TX_NULL,
        VIKING_ROOM
    };

    explicit Texture(Kind kind) : LoadImage(get_texture_path(kind)) {
        m_kind = kind;
    }
    ~Texture() = default;

    [[nodiscard]] Kind get_kind() const { return m_kind; }

private:
    Kind m_kind;

    static const char* get_texture_path(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return TEXTURE_PATH.c_str();
            default:
                return "/Users/sebastian/CLionProjects/soundscape/textures/texture.jpg";
        }
        throw std::runtime_error("Unknown model kind");
    }
};

class TextureImage2 : public SamplerImage::SamplerImage {
public:
    explicit
    TextureImage2(std::shared_ptr<Device::Device> &device, VkCommandPool command_pool, Texture* texture) : SamplerImage(
        get_sampler_spec(device.get(), texture))
    {
        StagingBuffer::Spec staging_buffer_spec = {};
        staging_buffer_spec.device = get_device();
        staging_buffer_spec.data = texture->get_pixels();
        staging_buffer_spec.size = texture->get_image_size();
        auto staging_buffer = new StagingBuffer::StagingBuffer(staging_buffer_spec);

        transition_layout(command_pool, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copy_buffer_to_image(command_pool, staging_buffer->get_handle());
        generate_mipmaps(command_pool);

        delete staging_buffer;
    }
    ~TextureImage2() = default;
private:
    std::shared_ptr<Device::Device> m_device;

    static ::SamplerImage::Spec get_sampler_spec(Device::Device* device, const Texture* texture) {
        ::SamplerImage::Spec sampler_image_spec = {};
        sampler_image_spec.device = device;
        sampler_image_spec.width = texture->get_width();
        sampler_image_spec.height = texture->get_height();
        sampler_image_spec.num_samples = VK_SAMPLE_COUNT_1_BIT;
        sampler_image_spec.format = VK_FORMAT_R8G8B8A8_SRGB;
        sampler_image_spec.tiling = VK_IMAGE_TILING_OPTIMAL;
        sampler_image_spec.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        sampler_image_spec.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
        sampler_image_spec.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        sampler_image_spec.min_filter = VK_FILTER_LINEAR;
        sampler_image_spec.mag_filter = VK_FILTER_LINEAR;
        sampler_image_spec.address_modes = {
            VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT};
        sampler_image_spec.border_color = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_image_spec.un_normalized = VK_FALSE;
        sampler_image_spec.compare_enable = VK_FALSE;
        sampler_image_spec.compare_op = VK_COMPARE_OP_LESS;
        sampler_image_spec.mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_image_spec.min_lod = 0.0f;
        sampler_image_spec.mip_lod_bias = 0.0f;
        return sampler_image_spec;
    };
};

class TextureManager {
public:
    TextureManager(const std::shared_ptr<Device::Device> &device, VkCommandPool command_pool) : m_device(device),
        m_command_pool(command_pool) {}

    [[nodiscard]] std::shared_ptr<TextureImage2> acquire_texture(Texture::Kind kind) {
        if (!m_textures.contains(kind)) {
            const auto texture = new Texture(kind);
            const auto texture_image = std::make_shared<TextureImage2>(m_device, m_command_pool, texture);
            delete texture;
            m_textures[kind] = texture_image;
        }

        return m_textures[kind];
    }
private:
    std::shared_ptr<Device::Device> m_device;
    VkCommandPool m_command_pool;
    std::map<Texture::Kind, std::shared_ptr<TextureImage2>> m_textures;
};

#endif //TEXTURE_H
