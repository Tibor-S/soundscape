//
// Created by Sebastian Sandstig on 2024-12-03.
//

#ifndef PIPELINELAYOUT_H
#define PIPELINELAYOUT_H

#include <vulkan/vulkan.hpp>

#include <Descriptor.h>

class PipelineLayout : public Descriptor {
public:
    PipelineLayout(const std::shared_ptr<Device> &device,
                    const std::shared_ptr<DescriptorLayout> &layout) : Descriptor(layout->get_kind()), m_device(device),
                                                                       m_layout(layout)
    {
        VkPipelineLayoutCreateInfo pipeline_layout_info{};
        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount = 1;
        auto set_layout = m_layout->get_handle();
        pipeline_layout_info.pSetLayouts = &set_layout;

        if (vkCreatePipelineLayout(m_device->logical_device_handle(), &pipeline_layout_info, nullptr,
                                   &m_pipeline_layout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    };
    ~PipelineLayout() {
        vkDestroyPipelineLayout(m_device->logical_device_handle(), m_pipeline_layout, nullptr);
    }

    [[nodiscard]] VkPipelineLayout get_handle() const { return m_pipeline_layout; }
private:
    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
    std::shared_ptr<Device> m_device;
    std::shared_ptr<DescriptorLayout> m_layout;
};

class PipelineLayoutManager {
public:
    PipelineLayoutManager(const std::shared_ptr<Device>& device,
                          const std::shared_ptr<DescriptorLayoutManager>& descriptor_layout_manager,
                          VkCommandPool command_pool) : m_device(device),
                                                        m_descriptor_layout_manager(descriptor_layout_manager),
                                                        m_command_pool(command_pool) {
    }

    [[nodiscard]] std::shared_ptr<PipelineLayout> acquire_pipeline_layout(PipelineLayout::Kind kind) {
        if (!m_pipeline_layout.contains(kind)) {
            auto descriptor_layout = m_descriptor_layout_manager->acquire_descriptor_layout(kind);
            const auto pipeline_layout = std::make_shared<PipelineLayout>(m_device, descriptor_layout);
            m_pipeline_layout[kind] = pipeline_layout;
        }
        return m_pipeline_layout[kind];
    }
private:
    std::shared_ptr<Device> m_device;
    std::shared_ptr<DescriptorLayoutManager> m_descriptor_layout_manager;
    VkCommandPool m_command_pool;
    std::map<PipelineLayout::Kind, std::shared_ptr<PipelineLayout>> m_pipeline_layout;
};

#endif //PIPELINELAYOUT_H
