//
// Created by Sebastian Sandstig on 2024-12-03.
//

#ifndef PIPELINELAYOUT_H
#define PIPELINELAYOUT_H
#include <vulkan/vulkan.hpp>
#include <DescriptorManager.h>

namespace PipelineLayout {

struct Spec {
    Descriptor::DescriptorManager* descriptor_manager;
};

class PipelineLayout : public Descriptor::DescriptorManagerParent {
public:
    explicit PipelineLayout(Spec& spec);
    ~PipelineLayout();

    VkPipelineLayout get_handle() const { return m_pipeline_layout; }
private:
    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
};

class PipelineLayoutParent {
public:
    explicit PipelineLayoutParent(PipelineLayout* pipeline_layout) { m_pipeline_layout = pipeline_layout; }
    PipelineLayout* get_pipeline_layout() const { return m_pipeline_layout; }
private:
    PipelineLayout* m_pipeline_layout;
};

} // PipelineLayout

#endif //PIPELINELAYOUT_H
