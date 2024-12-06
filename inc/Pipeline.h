//
// Created by Sebastian Sandstig on 2024-12-03.
//

#ifndef PIPELINE_H
#define PIPELINE_H
#include <PipelineLayout.h>
#include <RenderTarget.h>
#include <Vertex.h>

namespace Pipeline {


struct Spec {
    RenderTarget::RenderTarget* render_target;
    PipelineLayout::PipelineLayout* pipeline_layout;

    std::string vert_code_path;
    std::string frag_code_path;
};

class Pipeline : PipelineLayout::PipelineLayoutParent, RenderTarget::RenderTargetParent {
public:
    explicit Pipeline(Spec& spec);
    ~Pipeline();

    VkPipeline get_handle() const { return m_pipeline; }
private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    static VkShaderModule create_shader_module(Device::Device* device, const std::vector<char>& code);
};

static std::vector<char> read_file(const std::string& filename);

} // Pipeline



#endif //PIPELINE_H
