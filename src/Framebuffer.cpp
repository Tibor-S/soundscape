//
// Created by Sebastian Sandstig on 2024-12-02.
//

#include "Framebuffer.h"

namespace Framebuffer {

Framebuffer::Framebuffer(Spec& spec) : SwapChainParent(spec.swap_chain), RenderPassParent(spec.render_pass) {
    m_swap_chain_index = spec.swap_chain_index;

    auto render_pass = get_render_pass();
    auto device = render_pass->get_device();
    auto color_image = get_swap_chain()->get_color_image();
    auto depth_image = get_swap_chain()->get_depth_image();
    auto extent = get_swap_chain()->get_extent();

    std::array<VkImageView, 3> attachments = {
        color_image->get_image_view(),
        depth_image->get_image_view(),
        get_swap_chain()->get_image_view(m_swap_chain_index)
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = render_pass->get_render_pass_handle();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device->logical_device_handle(), &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

Framebuffer::~Framebuffer() {
    auto render_pass = get_render_pass();
    auto device = render_pass->get_device();
    vkDeviceWaitIdle(device->logical_device_handle());
    vkDestroyFramebuffer(device->logical_device_handle(), m_framebuffer, nullptr);
}

} // Framebuffer