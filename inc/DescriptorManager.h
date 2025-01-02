//
// Created by Sebastian Sandstig on 2024-11-27.
//

#ifndef DESCRIPTORMANAGER_H
#define DESCRIPTORMANAGER_H

#include <vector>

#include <vulkan/vulkan.hpp>

#include <Device.h>

enum DescriptorType {
    UNIFORM_BUFFER = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    IMAGE_SAMPLER = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
};
enum DescriptorShaderStage {
    VERTEX = VK_SHADER_STAGE_VERTEX_BIT,
    FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT,
};
struct DescriptionSpec {
    DescriptorType type;
    size_t binding;
    size_t count;
    std::vector<DescriptorShaderStage> shader_stages;
};
struct DescriptorPoolSpec {
    Device* device;
    std::vector<DescriptionSpec> desc_bindings;
};
class DescriptorUpdater;

class DescriptorManager : public DeviceParent {
public:
    DescriptorManager(DescriptorPoolSpec &spec, size_t max_sets);
    ~DescriptorManager();

    std::vector<size_t> get_unique_descriptors(size_t count);
    [[nodiscard]] VkDescriptorSetLayout get_layout_handle() const;
    [[nodiscard]] VkDescriptorSet descriptor_handle(size_t index) const;
    [[nodiscard]] DescriptorType descriptor_type(size_t binding) const;
    DescriptorUpdater* get_updater();

private:
    VkDescriptorPool m_descriptor_pool {};
    VkDescriptorSetLayout m_descriptor_set_layout {};
    std::vector<VkDescriptorSet> m_descriptor_sets;
    std::vector<DescriptionSpec> m_desc_bindings;
};


class DescriptorUpdater {
public:
    DescriptorManager *manager = nullptr; // Parent

    explicit DescriptorUpdater(DescriptorManager *manager) {
        this->manager = manager;
    };
    void update(); // Frees Updater
    DescriptorUpdater* update_buffer(size_t index, size_t binding, VkBuffer buffer_handle, size_t offset, size_t size);
    DescriptorUpdater* update_image(size_t index, size_t binding, VkImageView image_view_handle, VkSampler sampler_handle);
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

#endif //DESCRIPTORMANAGER_H
