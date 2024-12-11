//
// Created by Sebastian Sandstig on 2024-11-27.
//

#include <DescriptorManager.h>
#include <iostream>
namespace Descriptor {
// DescriptorManager

DescriptorManager::DescriptorManager(DescPoolSpec &spec, size_t max_sets) : DeviceParent(spec.device) {
    m_desc_bindings = spec.desc_bindings;
    std::vector<VkDescriptorPoolSize> poolSizes{};
    poolSizes.resize(spec.desc_bindings.size());
    for (size_t i = 0; i < spec.desc_bindings.size(); i++) {
        auto [type,
            _b,
            count,
            _s] = spec.desc_bindings[i];

        poolSizes[i].type = static_cast<VkDescriptorType>(type);
        poolSizes[i].descriptorCount = count * max_sets;
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(max_sets);

    if (vkCreateDescriptorPool(get_device()->logical_device_handle(), &poolInfo, nullptr,
        &m_descriptor_pool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.resize(spec.desc_bindings.size());
    for (size_t i = 0; i < spec.desc_bindings.size(); i++) {
        auto [type,
            binding,
            count,
            shader_stages] = spec.desc_bindings[i];

        VkShaderStageFlags vk_shader_stage = 0;
        for (const auto shader_stage : shader_stages) {
            vk_shader_stage |= shader_stage;
        }

        VkDescriptorSetLayoutBinding layout_spec{};
        layout_spec.binding = binding;
        layout_spec.descriptorCount = 1;
        layout_spec.descriptorType = static_cast<VkDescriptorType>(type);
        layout_spec.pImmutableSamplers = nullptr;
        layout_spec.stageFlags = vk_shader_stage;

        bindings[i] = layout_spec;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(spec.device->logical_device_handle(), &layoutInfo, nullptr,
        &m_descriptor_set_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    m_descriptor_sets.resize(max_sets, {});
}

DescriptorManager::~DescriptorManager() {
    vkDestroyDescriptorPool(get_device()->logical_device_handle(), m_descriptor_pool, nullptr);

    vkDestroyDescriptorSetLayout(get_device()->logical_device_handle(), m_descriptor_set_layout, nullptr);
}


std::vector<size_t> DescriptorManager::get_unique_descriptors(size_t count) {
    std::vector<size_t> unique_descriptors;

    for (size_t i = 0; i < m_descriptor_sets.size(); i++) {
        if (m_descriptor_sets[i] == VK_NULL_HANDLE) {
            unique_descriptors.push_back(i);
        }
    }

    std::vector<VkDescriptorSetLayout> layouts(count, m_descriptor_set_layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptor_pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> temp_hold;
    temp_hold.resize(count);
    if (vkAllocateDescriptorSets(get_device()->logical_device_handle(), &allocInfo, temp_hold.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < count; i++) {
        size_t index = unique_descriptors[i];
        m_descriptor_sets[index] = temp_hold[i];
    }

    return unique_descriptors;
}

// Device::Device* DescriptorManager::device() const {
//     return m_spec.device;
// }

VkDescriptorSetLayout DescriptorManager::get_layout_handle() {
    return m_descriptor_set_layout;
}

VkDescriptorSet DescriptorManager::descriptor_handle(size_t index) const {
    return m_descriptor_sets[index];
}

Type DescriptorManager::descriptor_type(size_t binding) const {
    for (auto & desc_binding : m_desc_bindings) {
        if (desc_binding.binding == binding) {
            return desc_binding.type;
        }
    }
    throw std::runtime_error("binding mismatch!");
}

Updater* DescriptorManager::get_updater() {
    auto* updater = new Updater(this);

    return updater;
}

// Updater

void Updater::update() {
    std::vector<VkDescriptorBufferInfo> buffers_info {};
    std::vector<VkDescriptorImageInfo> images_info {};
    std::vector<VkWriteDescriptorSet> writes_info {};
    buffers_info.resize(m_buffers.size());
    images_info.resize(m_images.size());
    writes_info.resize(m_buffers.size() + m_images.size());

    size_t pre = 0;
    for (size_t i = 0; i < buffers_info.size(); i++) {
        auto [descriptor_index,
            binding,
            buffer_handle,
            offset,
            size] = m_buffers[i];

        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = buffer_handle;
        buffer_info.offset = static_cast<VkDeviceSize>(offset);
        buffer_info.range = static_cast<VkDeviceSize>(size);
        buffers_info[i] = buffer_info;

        VkWriteDescriptorSet write_info{};
        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_info.dstSet = manager->descriptor_handle(descriptor_index);
        write_info.dstBinding = static_cast<uint32_t>(binding);
        write_info.dstArrayElement = 0;
        write_info.descriptorType = static_cast<VkDescriptorType>(manager->descriptor_type(binding));
        write_info.descriptorCount = 1;
        write_info.pBufferInfo = &buffers_info[i];
        writes_info[pre + i] = write_info;
    }

    pre += buffers_info.size();
    for (size_t i = 0; i < images_info.size(); i++) {
        auto [descriptor_index,
            binding,
            image_view_handle,
            sampler_handle] = m_images[i];

        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image_view_handle;
        image_info.sampler = sampler_handle;
        images_info[i] = image_info;

        VkWriteDescriptorSet write_info{};
        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_info.dstSet = manager->descriptor_handle(descriptor_index);
        write_info.dstBinding = static_cast<uint32_t>(binding);
        write_info.dstArrayElement = 0;
        write_info.descriptorType = static_cast<VkDescriptorType>(manager->descriptor_type(binding));
        write_info.descriptorCount = 1;
        write_info.pImageInfo = &images_info[0];
        writes_info[pre + i] = write_info;
        // throw std::runtime_error("failed to update descriptor sets!");
    }

    vkUpdateDescriptorSets(manager->get_device()->logical_device_handle(), static_cast<uint32_t>(writes_info.size()),
        writes_info.data(), 0, nullptr);
}

Updater* Updater::update_buffer(size_t index, size_t binding, VkBuffer buffer_handle, size_t offset, size_t size) {
    m_buffers.push_back({
        .descriptor_index = index,
        .binding = binding,
        .buffer_handle = buffer_handle,
        .offset = offset,
        .size = size,
    });

    return this;
}

Updater* Updater::update_image(size_t index, size_t binding, VkImageView image_view_handle, VkSampler sampler_handle) {
    m_images.push_back({
        .descriptor_index = index,
        .binding = binding,
        .image_view_handle = image_view_handle,
        .sampler_handle = sampler_handle,
    });

    return this;
}
}
