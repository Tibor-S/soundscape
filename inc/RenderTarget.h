//
// Created by Sebastian Sandstig on 2024-12-03.
//

#ifndef RENDERTARGET_H
#define RENDERTARGET_H
#include <Framebuffer.h>
#include <RenderPass.h>
#include <SwapChain.h>

namespace RenderTarget {

struct Spec : SwapChain::Spec {
    // size_t max_image_count;
};

class RenderTarget : public Device::DeviceParent {
public:
    explicit RenderTarget(Spec& spec);
    ~RenderTarget();
    void recreate_swap_chain();
    // void destroy();

    SwapChain::SwapChain* get_swap_chain() const { return m_swap_chain; }
    Framebuffer::Framebuffer* get_framebuffer(size_t index) const { return m_framebuffers[index]; }
    RenderPass::RenderPass* get_render_pass() const { return m_render_pass; }
    size_t get_image_count() const { return m_max_image_count; }

private:
    GLFWwindow* m_window;
    VkSurfaceKHR m_surface_handle;
    SwapChain::SwapChain* m_swap_chain;
    std::vector<Framebuffer::Framebuffer*> m_framebuffers;
    RenderPass::RenderPass* m_render_pass;
    size_t m_max_image_count;
};

class RenderTargetParent {
public:
    explicit RenderTargetParent(RenderTarget* render_target) { m_render_target = render_target; }
    RenderTarget* get_render_target() const { return m_render_target; }
private:
    RenderTarget* m_render_target;
};

} // Render

#endif //RENDERTARGET_H
