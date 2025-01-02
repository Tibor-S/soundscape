//
// Created by Sebastian Sandstig on 2024-12-02.
//

#ifndef RENDERPASS_H
#define RENDERPASS_H
#include <SwapChain.h>
#include <vulkan/vulkan.hpp>

namespace RenderPass {

struct Spec {
    Device* device;
    VkFormat color_format;
    VkFormat depth_format;
};

class RenderPass : public DeviceParent {
public:
    explicit RenderPass(Spec& spec);
    ~RenderPass();
    VkRenderPass get_render_pass_handle() const {return m_render_pass_handle;};
    VkFormat get_color_format() const {return m_color_format;};
    VkFormat get_depth_format() const {return m_depth_format;};
private:
    VkRenderPass m_render_pass_handle;
    VkFormat m_color_format;
    VkFormat m_depth_format;

    static VkRenderPass create_render_pass(Device* device, VkFormat color_format, VkFormat depth_format);
};

class RenderPassParent {
public:
    RenderPassParent(RenderPass* render_pass) { m_render_pass = render_pass; };
    RenderPass* get_render_pass() const { return m_render_pass; };
private:
    RenderPass* m_render_pass;
};

} // RenderPass

#endif //RENDERPASS_H
