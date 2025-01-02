//
// Created by Sebastian Sandstig on 2024-12-03.
//

#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include <Framebuffer.h>
#include <RenderPass.h>
#include <SwapChain.h>

struct RenderTargetSpec : SwapChainSpec {};

class RenderTarget : public DeviceParent {
public:
    explicit RenderTarget(RenderTargetSpec& spec);
    ~RenderTarget();
    void recreate_swap_chain();

    [[nodiscard]] SwapChain* get_swap_chain() const { return m_swap_chain; }
    [[nodiscard]] Framebuffer* get_framebuffer(size_t index) const { return m_framebuffers[index]; }
    [[nodiscard]] RenderPass* get_render_pass() const { return m_render_pass; }
    [[nodiscard]] size_t get_image_count() const { return m_max_image_count; }

private:
    GLFWwindow* m_window;
    VkSurfaceKHR m_surface_handle;
    SwapChain* m_swap_chain;
    std::vector<Framebuffer*> m_framebuffers;
    RenderPass* m_render_pass;
    size_t m_max_image_count;
};

class RenderTargetParent {
public:
    explicit RenderTargetParent(RenderTarget* render_target) { m_render_target = render_target; }
    [[nodiscard]] RenderTarget* get_render_target() const { return m_render_target; }
private:
    RenderTarget* m_render_target;
};

#endif //RENDERTARGET_H
