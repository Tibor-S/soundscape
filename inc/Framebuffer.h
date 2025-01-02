//
// Created by Sebastian Sandstig on 2024-12-02.
//

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <vulkan/vulkan.hpp>

#include <RenderPass.h>

struct FramebufferSpec {
    RenderPass* render_pass;
    SwapChain* swap_chain;
    size_t swap_chain_index;
};

class Framebuffer : SwapChainParent, RenderPassParent {
public:
    explicit Framebuffer(const FramebufferSpec& spec);
    ~Framebuffer();
    [[nodiscard]] VkFramebuffer get_framebuffer_handle() const { return m_framebuffer; };
    [[nodiscard]] size_t get_swap_chain_index() const { return m_swap_chain_index; };
private:
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    size_t m_swap_chain_index;
};

#endif //FRAMEBUFFER_H
