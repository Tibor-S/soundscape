//
// Created by Sebastian Sandstig on 2024-12-03.
//

#include "PipelineLayout.h"

namespace PipelineLayout {
PipelineLayout::PipelineLayout(Spec& spec) : DescriptorManagerParent(spec.descriptor_manager) {
    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    auto layout = get_descriptor_manager()->get_layout_handle();
    pipeline_layout_info.pSetLayouts = &layout;

    auto device = get_descriptor_manager()->get_device();
    if (vkCreatePipelineLayout(device->logical_device_handle(), &pipeline_layout_info, nullptr, &m_pipeline_layout) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
}

PipelineLayout::~PipelineLayout() {
    vkDestroyPipelineLayout(get_descriptor_manager()->get_device()->logical_device_handle(), m_pipeline_layout,
                            nullptr);
}


} // PipelineLayout