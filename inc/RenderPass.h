//
// Created by Sebastian Sandstig on 2024-12-02.
//

#ifndef RENDERPASS_H
#define RENDERPASS_H

#include <vulkan/vulkan.hpp>

#include <SwapChain.h>

struct RenderPassSpec {
    Device* device;
    VkFormat color_format;
    VkFormat depth_format;
};

class RenderPass : public DeviceParent {
public:
    explicit RenderPass(const RenderPassSpec& spec);
    ~RenderPass();
    [[nodiscard]] VkRenderPass get_render_pass_handle() const {return m_render_pass_handle;};
    [[nodiscard]] VkFormat get_color_format() const {return m_color_format;};
    [[nodiscard]] VkFormat get_depth_format() const {return m_depth_format;};
private:
    VkRenderPass m_render_pass_handle;
    VkFormat m_color_format;
    VkFormat m_depth_format;

    static VkRenderPass create_render_pass(Device* device, VkFormat color_format, VkFormat depth_format);
};

class RenderPassParent {
public:
    explicit RenderPassParent(RenderPass* render_pass) { m_render_pass = render_pass; };
    [[nodiscard]] RenderPass* get_render_pass() const { return m_render_pass; };
private:
    RenderPass* m_render_pass;
};

#endif //RENDERPASS_H
