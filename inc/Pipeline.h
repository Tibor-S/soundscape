//
// Created by Sebastian Sandstig on 2024-12-03.
//

#ifndef PIPELINE_H
#define PIPELINE_H
#include <PipelineLayout.h>
#include <RenderTarget.h>

namespace Pipeline {
struct Spec {
    RenderTarget::RenderTarget* render_target;
    PipelineLayout::PipelineLayout* pipeline_layout;

    std::string vert_code_path;
    std::string frag_code_path;
};

class Pipeline : public PipelineLayout::PipelineLayoutParent, public RenderTarget::RenderTargetParent {
public:
    explicit Pipeline(Spec& spec);
    ~Pipeline();

    [[nodiscard]] VkPipeline get_handle() const { return m_pipeline; }
private:
    VkPipeline m_pipeline = VK_NULL_HANDLE;

};

class PipelineParent {
public:
    explicit PipelineParent(Pipeline* pipeline) {m_pipeline = pipeline;}
    [[nodiscard]] Pipeline* get_pipeline() const {return m_pipeline;}
private:
    Pipeline* m_pipeline;
};


} // Pipeline

VkShaderModule create_shader_module(Device::Device* device, const std::vector<char>& code);
std::vector<char> read_file(const std::string& filename);


// class PipelineLayout2 {
// public:
//     enum Kind {
//
//     };
// };

class Pipeline2 {
public:
    enum Kind {
        STANDARD,
        BAR,
    };

    Pipeline2(std::shared_ptr<Device::Device> &device, std::shared_ptr<PipelineLayout2> &layout,
              std::shared_ptr<RenderTarget::RenderTarget> &render_target, const Kind kind) : m_kind(kind),
        m_device(device), m_layout(layout), m_render_target(render_target)
    {
        auto render_pass = m_render_target->get_render_pass();

        auto vert_shader_code = read_file(get_vert_code_path());
        auto frag_shader_code = read_file(get_frag_code_path());

        VkShaderModule vert_shader_module = create_shader_module(m_device.get(), vert_shader_code);
        VkShaderModule frag_shader_module = create_shader_module(m_device.get(), frag_shader_code);

        VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
        vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vert_shader_stage_info.module = vert_shader_module;
        vert_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
        frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        frag_shader_stage_info.module = frag_shader_module;
        frag_shader_stage_info.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vert_shader_stage_info, frag_shader_stage_info};

        VkPipelineVertexInputStateCreateInfo vertex_input_info{};
        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto binding_description = layout->get_binding_description();
        auto attribute_descriptions = layout->get_attribute_descriptions();

        vertex_input_info.vertexBindingDescriptionCount = 1;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
        vertex_input_info.pVertexBindingDescriptions = &binding_description;
        vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly{};
        input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
        multisampling.rasterizationSamples = m_device->max_sample_count();
        multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one

        VkPipelineColorBlendAttachmentState color_blend_attachment{};
        color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable = VK_FALSE;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0f;
        color_blending.blendConstants[1] = 0.0f;
        color_blending.blendConstants[2] = 0.0f;
        color_blending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        VkPipelineDepthStencilStateCreateInfo depth_stencil{};
        depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth_stencil.depthTestEnable = VK_TRUE;
        depth_stencil.depthWriteEnable = VK_TRUE;
        depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depth_stencil.depthBoundsTestEnable = VK_FALSE;
        depth_stencil.minDepthBounds = 0.0f; // Optional
        depth_stencil.maxDepthBounds = 1.0f; // Optional
        depth_stencil.stencilTestEnable = VK_FALSE;
        depth_stencil.front = {}; // Optional
        depth_stencil.back = {}; // Optional

        VkGraphicsPipelineCreateInfo pipeline_info{};
        pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline_info.stageCount = 2;
        pipeline_info.pStages = shaderStages;
        pipeline_info.pVertexInputState = &vertex_input_info;
        pipeline_info.pInputAssemblyState = &input_assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &rasterizer;
        pipeline_info.pMultisampleState = &multisampling;
        pipeline_info.pColorBlendState = &color_blending;
        pipeline_info.pDepthStencilState = &depth_stencil;
        pipeline_info.pDynamicState = &dynamic_state;
        pipeline_info.layout = m_layout->get_handle();
        pipeline_info.renderPass = render_pass->get_render_pass_handle();
        pipeline_info.subpass = 0;
        pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(m_device->logical_device_handle(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
                                      &m_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device->logical_device_handle(), frag_shader_module, nullptr);
        vkDestroyShaderModule(device->logical_device_handle(), vert_shader_module, nullptr);
    }
    ~Pipeline2() = default;

    [[nodiscard]] VkPipeline get_handle() const { return m_pipeline; }
    static PipelineLayout2::Kind get_layout_kind(const Kind kind) {
        switch (kind) {
            case STANDARD:
                return PipelineLayout2::CAMERA_MODEL_SAMPLER;
            case BAR:
                return PipelineLayout2::CAMERA_MODEL_BUFFER;
        }
        throw std::invalid_argument("Invalid kind");
    }

private:
    Kind m_kind;
    std::shared_ptr<Device::Device> m_device;
    std::shared_ptr<PipelineLayout2> m_layout;
    std::shared_ptr<RenderTarget::RenderTarget> m_render_target;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    [[nodiscard]] const char* get_vert_code_path() const {
        switch (m_kind) {
            case Kind::STANDARD:
                return "/Users/sebastian/CLionProjects/soundscape/shaders/vert.spv";
            case BAR:
                return "/Users/sebastian/CLionProjects/soundscape/shaders/bar_vert.spv";
        }
        throw std::invalid_argument("Invalid kind");
    }

    [[nodiscard]] const char* get_frag_code_path() const {
        switch (m_kind) {
            case Kind::STANDARD:
                return "/Users/sebastian/CLionProjects/soundscape/shaders/frag.spv";
            case BAR:
                return "/Users/sebastian/CLionProjects/soundscape/shaders/bar_frag.spv";
        }
        throw std::invalid_argument("Invalid kind");
    }
};

class PipelineManager {
public:
    PipelineManager(const std::shared_ptr<Device::Device>& device,
                          const std::shared_ptr<PipelineLayoutManager>& pipeline_layout_manager,
                          const std::shared_ptr<RenderTarget::RenderTarget>& render_target,
                          VkCommandPool command_pool) : m_device(device),
                                                        m_render_target(render_target),
                                                        m_pipeline_layout_manager(pipeline_layout_manager),
                                                        m_command_pool(command_pool) {
    }

    [[nodiscard]] std::shared_ptr<Pipeline2> acquire_pipeline(Pipeline2::Kind kind) {
        if (!m_pipeline.contains(kind)) {
            auto layout_kind = Pipeline2::get_layout_kind(kind);
            auto pipeline_layout = m_pipeline_layout_manager->acquire_pipeline_layout(layout_kind);
            const auto pipeline = std::make_shared<Pipeline2>(m_device, pipeline_layout, m_render_target, kind);
            m_pipeline[kind] = pipeline;
        }

        return m_pipeline[kind];
    }
private:
    std::shared_ptr<Device::Device> m_device;
    std::shared_ptr<PipelineLayoutManager> m_pipeline_layout_manager;
    std::shared_ptr<RenderTarget::RenderTarget> m_render_target;
    VkCommandPool m_command_pool;
    std::map<Pipeline2::Kind, std::shared_ptr<Pipeline2>> m_pipeline;
};

#endif //PIPELINE_H
