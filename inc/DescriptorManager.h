//
// Created by Sebastian Sandstig on 2024-11-27.
//

#ifndef DESCRIPTORMANAGER_H
#define DESCRIPTORMANAGER_H
#include <Device.h>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Descriptor {

enum Type {
    UNIFORM_BUFFER = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    IMAGE_SAMPLER = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
};
enum ShaderStage {
    VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
    FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
};
struct DescSpec {
    Type type;
    size_t binding;
    size_t count;
    std::vector<ShaderStage> shader_stages;
};
struct DescPoolSpec {
    Device::Device* device;
    std::vector<DescSpec> desc_bindings;
};
class Updater;

class DescriptorManager : public Device::DeviceParent {
public:
    DescriptorManager(DescPoolSpec &spec, size_t max_sets);
    ~DescriptorManager();

    std::vector<size_t> get_unique_descriptors(size_t count);
    VkDescriptorSetLayout get_layout_handle();
    VkDescriptorSet descriptor_handle(size_t index) const;
    Type descriptor_type(size_t binding) const;
    Updater* get_updater();
    // void destroy() const;

private:
    VkDescriptorPool m_descriptor_pool {};
    VkDescriptorSetLayout m_descriptor_set_layout {};
    std::vector<VkDescriptorSet> m_descriptor_sets;
    std::vector<DescSpec> m_desc_bindings;
};


class Updater {
public:
    DescriptorManager *manager = nullptr; // Parent

    explicit Updater(DescriptorManager *manager) {
        this->manager = manager;
    };
    void update(); // Frees Updater
    Updater* update_buffer(size_t index, size_t binding, VkBuffer buffer_handle, size_t offset, size_t size);
    Updater* update_image(size_t index, size_t binding, VkImageView image_view_handle, VkSampler sampler_handle);
private:
    struct UpdateBuffer {
        size_t descriptor_index;
        size_t binding;
        VkBuffer buffer_handle;
        size_t offset;
        size_t size;
    };
    struct UpdateImage {
        size_t descriptor_index;
        size_t binding;
        VkImageView image_view_handle;
        VkSampler sampler_handle;
    };


    // Buffer
    std::vector<UpdateBuffer> m_buffers;
    std::vector<UpdateImage> m_images;
};

class DescriptorManagerParent {
public:
    explicit DescriptorManagerParent(DescriptorManager *descriptor_manager) {
        m_descriptor_manager = descriptor_manager;
    }
    [[nodiscard]] DescriptorManager* get_descriptor_manager() const {return m_descriptor_manager;}
private:
    DescriptorManager* m_descriptor_manager;
};
}

#endif //DESCRIPTORMANAGER_H
