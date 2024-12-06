//
// Created by Sebastian Sandstig on 2024-12-02.
//

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <RenderPass.h>
#include <vulkan/vulkan.hpp>

namespace Framebuffer  {

struct Spec {
    RenderPass::RenderPass* render_pass;
    SwapChain::SwapChain* swap_chain;
    size_t swap_chain_index;
};

class Framebuffer : SwapChain::SwapChainParent, RenderPass::RenderPassParent {
public:
    explicit Framebuffer(Spec& spec);
    ~Framebuffer();
    VkFramebuffer get_framebuffer_handle() const { return m_framebuffer; };
    size_t get_swap_chain_index() const { return m_swap_chain_index; };
private:
    VkFramebuffer m_framebuffer;
    size_t m_swap_chain_index;
};

} // Framebuffer

#endif //FRAMEBUFFER_H
