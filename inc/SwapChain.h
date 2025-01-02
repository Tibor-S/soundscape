//
// Created by Sebastian Sandstig on 2024-12-01.
//

#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <Device.h>
#include <Image.h>


struct SwapChainSpec {
    Device* device;
    GLFWwindow* window;
    VkSurfaceKHR surface_handle;
    size_t max_image_count;
};


class SwapChain : public DeviceParent {
public:
    explicit SwapChain(const SwapChainSpec &spec);
    ~SwapChain();

    [[nodiscard]] VkSwapchainKHR get_handle() const { return m_swap_chain_handle; }
    [[nodiscard]] VkImageView get_image_view(size_t index) const { return m_image_views[index]; }
    [[nodiscard]] Image* get_color_image() const { return m_color_image; }
    [[nodiscard]] Image* get_depth_image() const { return m_depth_image; }
    [[nodiscard]] VkExtent2D get_extent() const { return m_extent; }
    [[nodiscard]] VkSurfaceFormatKHR get_surface_format() const { return m_surface_format; }
    [[nodiscard]] VkFormat find_depth_format() const;
    [[nodiscard]] size_t get_image_count() const { return m_images.size(); }
private:
    GLFWwindow* m_window;
    VkSurfaceKHR m_surface_handle;
    VkSwapchainKHR m_swap_chain_handle;
    VkExtent2D m_extent = {};
    VkSurfaceFormatKHR m_surface_format = {};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
    Image* m_color_image;
    Image* m_depth_image;
    VkRenderPass m_render_pass_handle{};

    static VkSwapchainKHR create_swap_chain(const Device *device, VkSurfaceKHR surface_handle, GLFWwindow *window,
                                            const SwapChainSupportDetails *swap_chain_support,
                                            VkSurfaceFormatKHR *surface_format, const VkExtent2D *extent,
                                            VkPresentModeKHR present_mode, size_t max_image_count);
    static std::vector<VkImage> create_images(VkDevice device_handle, VkSwapchainKHR swap_chain_handle);
    static std::vector<VkImageView> create_image_views(VkDevice device_handle, const std::vector<VkImage> &images,
                                                       VkFormat format);
    static VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    static VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    static Image* create_color_image(Device* device, VkSurfaceFormatKHR surface_format, VkExtent2D extent);
    static Image* create_depth_image(Device* device, VkFormat depth_format, VkExtent2D extent);
};

class SwapChainParent {
public:
    explicit SwapChainParent(SwapChain* swap_chain) { m_swap_chain = swap_chain; };
    [[nodiscard]] SwapChain* get_swap_chain() const { return m_swap_chain; };
private:
    SwapChain* m_swap_chain;
};

#endif //SWAPCHAIN_H
