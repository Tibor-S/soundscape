//
// Created by Sebastian Sandstig on 2024-12-02.
//

#include <Framebuffer.h>

Framebuffer::Framebuffer(const FramebufferSpec &spec) : SwapChainParent(spec.swap_chain),
                                                        RenderPassParent(spec.render_pass)
{
    m_swap_chain_index = spec.swap_chain_index;

    const auto render_pass = get_render_pass();
    const auto device = render_pass->get_device();
    const auto color_image = get_swap_chain()->get_color_image();
    const auto depth_image = get_swap_chain()->get_depth_image();
    const auto extent = get_swap_chain()->get_extent();

    const std::array attachments = {
        color_image->get_image_view(),
        depth_image->get_image_view(),
        get_swap_chain()->get_image_view(m_swap_chain_index)
    };

    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = render_pass->get_render_pass_handle();
    framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = extent.width;
    framebuffer_info.height = extent.height;
    framebuffer_info.layers = 1;

    if (vkCreateFramebuffer(device->logical_device_handle(), &framebuffer_info,
        nullptr, &m_framebuffer) != VK_SUCCESS)
        {
        throw std::runtime_error("failed to create framebuffer!");
    }
}

Framebuffer::~Framebuffer() {
    const auto render_pass = get_render_pass();
    const auto device = render_pass->get_device();
    vkDeviceWaitIdle(device->logical_device_handle());
    vkDestroyFramebuffer(device->logical_device_handle(), m_framebuffer, nullptr);

}