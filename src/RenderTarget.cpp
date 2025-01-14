//
// Created by Sebastian Sandstig on 2024-12-03.
//

#include <RenderTarget.h>

RenderTarget::RenderTarget(RenderTargetSpec& spec) : DeviceParent(spec.device) {
    m_window = spec.window;
    m_surface_handle = spec.surface_handle;
    m_swap_chain = new SwapChain(spec);

    RenderPassSpec render_pass_spec{};
    render_pass_spec.device = get_device();
    render_pass_spec.color_format = m_swap_chain->get_surface_format().format;
    render_pass_spec.depth_format = m_swap_chain->find_depth_format();
    m_render_pass = new RenderPass(render_pass_spec);

    FramebufferSpec framebuffer_spec{};
    framebuffer_spec.render_pass = m_render_pass;
    framebuffer_spec.swap_chain = m_swap_chain;
    m_max_image_count = std::min(m_swap_chain->get_image_count(), spec.max_image_count);
    m_framebuffers.resize(m_max_image_count);
    for (size_t i = 0; i < m_framebuffers.size(); i++) {
        framebuffer_spec.swap_chain_index = i;
        m_framebuffers[i] = new Framebuffer(framebuffer_spec);
    }
}

RenderTarget::~RenderTarget() {
    for (auto const framebuffer : m_framebuffers) {
        delete framebuffer;
    }
    delete m_render_pass;
    delete m_swap_chain;
}


void RenderTarget::recreate_swap_chain() {
    for (auto const framebuffer : m_framebuffers) {
        delete framebuffer;
    }
    delete m_swap_chain;

    SwapChainSpec swap_chain_spec{};
    swap_chain_spec.device = get_device();
    swap_chain_spec.window = m_window;
    swap_chain_spec.surface_handle = m_surface_handle;
    swap_chain_spec.max_image_count = m_max_image_count;
    m_swap_chain = new SwapChain(swap_chain_spec);

    FramebufferSpec framebuffer_spec{};
    framebuffer_spec.render_pass = m_render_pass;
    framebuffer_spec.swap_chain = m_swap_chain;
    m_max_image_count = std::min(m_swap_chain->get_image_count(), m_max_image_count);
    m_framebuffers.resize(m_max_image_count);
    for (size_t i = 0; i < m_framebuffers.size(); i++) {
        framebuffer_spec.swap_chain_index = i;
        m_framebuffers[i] = new Framebuffer(framebuffer_spec);
    }
}
