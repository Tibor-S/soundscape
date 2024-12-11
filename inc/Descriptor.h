//
// Created by Sebastian Sandstig on 2024-12-06.
//

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H
#include <DescriptorManager.h>
#include <SamplerImage.h>
#include <UniformBuffer.h>


class ModelDescriptor {
public:
    glm::mat4 model;
};

class Descriptor2 {
public:
    enum Binding {
        CAMERA,
        MODEL,
        SAMPLER,
        UNIFORM_BUFFER
    };

    enum Kind {
        CAMERA_MODEL_SAMPLER
    };

    explicit Descriptor2(Kind kind) {
        m_kind = kind;
        set_bindings();
    }
    ~Descriptor2() = default;

    [[nodiscard]] Kind get_kind() const { return m_kind; }
    [[nodiscard]] Binding get_binding(size_t index) const { return m_bindings[index]; }
    [[nodiscard]] VkShaderStageFlags get_shader_stages(size_t index) const { return m_shader_stages[index]; }
    [[nodiscard]] size_t get_binding_count() const { return m_bindings.size(); }
    [[nodiscard]] static VkDescriptorType binding_type(Binding binding) {
        switch (binding) {
            case CAMERA:
            case MODEL:
            case UNIFORM_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case SAMPLER:
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        }
        throw std::runtime_error("Invalid binding type");
    }
private:
    Kind m_kind;
    std::vector<Binding> m_bindings;
    std::vector<VkShaderStageFlags> m_shader_stages;

    void set_bindings() {
        switch (m_kind) {
            case CAMERA_MODEL_SAMPLER:
                m_bindings = {CAMERA, MODEL, SAMPLER};
                m_shader_stages = {
                    VK_SHADER_STAGE_VERTEX_BIT,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                };
                return;
        }
        throw std::invalid_argument("invalid kind");
    }
};

class DescriptorLayout : public Descriptor2 {
public:
    DescriptorLayout(const std::shared_ptr<Device::Device>& device, Kind kind) : Descriptor2(kind), m_device(device) {
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.resize(get_binding_count());
        for (size_t i = 0; i < get_binding_count(); i++) {
            auto binding = get_binding(i);
            auto shader_stages = get_shader_stages(i);

            VkDescriptorSetLayoutBinding layout_spec{};
            layout_spec.binding = static_cast<uint32_t>(i);
            layout_spec.descriptorCount = 1;
            layout_spec.descriptorType = binding_type(binding);
            layout_spec.pImmutableSamplers = nullptr;
            layout_spec.stageFlags = shader_stages;

            bindings[i] = layout_spec;
        }

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device->logical_device_handle(), &layout_info, nullptr,
            &m_descriptor_set_layout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }
    ~DescriptorLayout() {
        vkDestroyDescriptorSetLayout(m_device->logical_device_handle(), m_descriptor_set_layout, nullptr);
    }

    [[nodiscard]] VkDescriptorSetLayout get_handle() const { return m_descriptor_set_layout; }
private:
    std::shared_ptr<Device::Device> m_device;
    VkDescriptorSetLayout m_descriptor_set_layout = VK_NULL_HANDLE;
};

class DescriptorLayoutManager {
public:
    DescriptorLayoutManager(const std::shared_ptr<Device::Device> &device,
                            VkCommandPool command_pool) : m_device(device),
                                                          m_command_pool(command_pool) {}

    [[nodiscard]] std::shared_ptr<DescriptorLayout> acquire_descriptor_layout(DescriptorLayout::Kind kind) {
        if (!m_descriptor_layout.contains(kind)) {
            const auto layout = std::make_shared<DescriptorLayout>(m_device, kind);
            m_descriptor_layout[kind] = layout;
        }
        return m_descriptor_layout[kind];
    }
private:
    std::shared_ptr<Device::Device> m_device;
    VkCommandPool m_command_pool;
    std::map<Descriptor2::Kind, std::shared_ptr<DescriptorLayout>> m_descriptor_layout;
};

class DescriptorPool : public Descriptor2 {
public:
    DescriptorPool(const std::shared_ptr<Device::Device> &device, std::shared_ptr<DescriptorLayout> &layout, Kind kind,
                   size_t max_sets) : Descriptor2(kind), m_device(device), m_descriptor_layout(layout)
    {
        std::map<Binding, uint32_t> binding_count {};
        for (size_t i = 0; i < get_binding_count(); i++) {
            if (auto binding = get_binding(i); binding_count.contains(binding)) {
                binding_count[binding]++;
            } else {
                binding_count[binding] = 1;
            }
        }

        std::vector<VkDescriptorPoolSize> pool_sizes{};
        for (const auto [binding, count] : binding_count) {
            VkDescriptorPoolSize pool_size{};
            pool_size.type = binding_type(binding);
            pool_size.descriptorCount = count * max_sets;
            pool_sizes.push_back(pool_size);
        }

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = static_cast<uint32_t>(max_sets);

        if (vkCreateDescriptorPool(m_device->logical_device_handle(), &pool_info, nullptr,
            &m_descriptor_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }
    ~DescriptorPool() {
        vkDestroyDescriptorPool(m_device->logical_device_handle(), m_descriptor_pool, nullptr);
        // delete m_device;
    }

    void free_descriptor_set(VkDescriptorSet descriptor_set) const {
        vkFreeDescriptorSets(m_device->logical_device_handle(), m_descriptor_pool, 1, &descriptor_set);
    }

    void update_descriptor_set(VkDescriptorSet descriptor_set,
                               const std::vector<VkWriteDescriptorSet> &write_info) const
    {
        vkUpdateDescriptorSets(m_device->logical_device_handle(), static_cast<uint32_t>(write_info.size()),
                               write_info.data(), 0, nullptr);
    }

    std::vector<VkDescriptorSet> allocate_descriptor_sets(const size_t count) const {
        std::vector<VkDescriptorSet> descriptor_sets{};
        descriptor_sets.resize(count);
        const std::vector layouts(count, m_descriptor_layout->get_handle());
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptor_pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(count);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(m_device->logical_device_handle(), &allocInfo, descriptor_sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        return descriptor_sets;
    }
    // [[nodiscard]] free_set(DescriptorSet*)
private:
    std::shared_ptr<Device::Device> m_device;
    std::shared_ptr<DescriptorLayout> m_descriptor_layout;
    VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
};



class DescriptorSet : public Descriptor2 {
public:
    explicit DescriptorSet(std::shared_ptr<DescriptorPool> &pool, VkDescriptorSet descriptor_set) : Descriptor2(
        pool->get_kind()), m_pool(pool), m_descriptor_set(descriptor_set) {}
    ~DescriptorSet() {
        m_pool->free_descriptor_set(m_descriptor_set);
        // delete m_pool;
    };

    static std::vector<std::shared_ptr<DescriptorSet>> create_descriptor_sets(std::shared_ptr<DescriptorPool> &pool, size_t count) {
        const auto handles = pool->allocate_descriptor_sets(count);
        std::vector<std::shared_ptr<DescriptorSet>> descriptor_sets{};
        descriptor_sets.resize(count);
        for (size_t i = 0; i < count; i++) {
            descriptor_sets[i] = std::make_shared<DescriptorSet>(pool, handles[i]);
        }
        return descriptor_sets;
    }

    [[nodiscard]] VkDescriptorSet get_descriptor_set() const { return m_descriptor_set; }

    void update(const std::vector<VkWriteDescriptorSet> &write_info) const {
        m_pool->update_descriptor_set(m_descriptor_set, write_info);
    }
private:
    std::shared_ptr<DescriptorPool> m_pool;
    VkDescriptorSet m_descriptor_set = VK_NULL_HANDLE;
};

class DescriptorSetUpdater {
public:
    explicit DescriptorSetUpdater(const std::shared_ptr<DescriptorSet> &descriptor_set) : m_descriptor_set(
        descriptor_set) {
        m_write_info.resize(m_descriptor_set->get_binding_count());
        m_buffer_info.resize(m_descriptor_set->get_binding_count());
        m_image_info.resize(m_descriptor_set->get_binding_count());
    }
    ~DescriptorSetUpdater() = default;

    void finalize() const {
        m_descriptor_set->update(m_write_info);
    }

    void update_buffer(size_t binding, const UniformBuffer::UniformBuffer& buffer) {
        const VkDescriptorBufferInfo buffer_info{
            .buffer = buffer.get_handle(),
            .offset = 0,
            .range = buffer.get_size()
        };
        m_buffer_info[binding] = buffer_info;

        const auto descriptor_binding = m_descriptor_set->get_binding(binding);
        VkWriteDescriptorSet write_info{};
        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_info.dstSet = m_descriptor_set->get_descriptor_set();
        write_info.dstBinding = static_cast<uint32_t>(binding);
        write_info.dstArrayElement = 0;
        write_info.descriptorType = Descriptor2::binding_type(descriptor_binding);
        write_info.descriptorCount = 1;
        write_info.pBufferInfo = &m_buffer_info[binding];
        m_write_info[binding] = write_info;
    }

    void update_image(size_t binding, const SamplerImage::SamplerImage& image) {
        const VkDescriptorImageInfo image_info{
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = image.get_image_view(),
            .sampler = image.get_sampler(),
        };
        m_image_info[binding] = image_info;

        const auto descriptor_binding = m_descriptor_set->get_binding(binding);
        VkWriteDescriptorSet write_info{};
        write_info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_info.dstSet = m_descriptor_set->get_descriptor_set();
        write_info.dstBinding = static_cast<uint32_t>(binding);
        write_info.dstArrayElement = 0;
        write_info.descriptorType = Descriptor2::binding_type(descriptor_binding);
        write_info.descriptorCount = 1;
        write_info.pImageInfo = &m_image_info[binding];
        m_write_info[binding] = write_info;
    }

    // void update_buffer(int binding, const Camera & camera);

private:
    std::shared_ptr<DescriptorSet> m_descriptor_set;
    std::vector<VkWriteDescriptorSet> m_write_info {};
    std::vector<VkDescriptorBufferInfo> m_buffer_info {};
    std::vector<VkDescriptorImageInfo> m_image_info {};
};


#endif //DESCRIPTOR_H
