//
// Created by Sebastian Sandstig on 2024-12-06.
//

#ifndef VISUAL_H
#define VISUAL_H

#include <iostream>
#include <map>

#include <Model.h>
#include <Sprite.h>
#include <Texture.h>

enum SpriteKind {
    VIKING_ROOM,
    BAR_SPRITE,
    BACK_DROP_SPRITE,
    COVER_ART_SPRITE
};

inline std::optional<Descriptor2::Kind> get_descriptor_kind(SpriteKind kind) {
    switch (kind) {
        case SpriteKind::VIKING_ROOM:
            return VikingRoom::descriptor_set_kind();
        case SpriteKind::BAR_SPRITE:
            return BarSprite::descriptor_set_kind();
        case SpriteKind::BACK_DROP_SPRITE:
            return BackDropSprite::descriptor_set_kind();
        case SpriteKind::COVER_ART_SPRITE:
            return CoverArtSprite::descriptor_set_kind();
    }
    throw std::invalid_argument("Invalid sprite kind");
}
inline std::optional<Pipeline2::Kind> get_pipeline_kind(SpriteKind kind) {
    switch (kind) {
        case SpriteKind::VIKING_ROOM:
            return VikingRoom::pipeline_kind();
        case SpriteKind::BAR_SPRITE:
            return BarSprite::pipeline_kind();
        case SpriteKind::BACK_DROP_SPRITE:
            return BackDropSprite::pipeline_kind();
        case SpriteKind::COVER_ART_SPRITE:
            return CoverArtSprite::pipeline_kind();
    }
    throw std::invalid_argument("Invalid sprite kind");
}
inline std::optional<Model::Kind> get_model_kind(SpriteKind kind) {
    switch (kind) {
        case SpriteKind::VIKING_ROOM:
            return VikingRoom::model_kind();
        case SpriteKind::BAR_SPRITE:
            return BarSprite::model_kind();
        case SpriteKind::BACK_DROP_SPRITE:
            return BackDropSprite::model_kind();
        case SpriteKind::COVER_ART_SPRITE:
            return CoverArtSprite::model_kind();
    }
    throw std::invalid_argument("Invalid sprite kind");
}
inline Sprite3* construct_sprite(std::shared_ptr<Device::Device> &device, TextureManager &texture_manager,
                                 PipelineManager &pipeline_manager,
                                 UniformBufferManager &uniform_buffer_manager,
                                 std::shared_ptr<VertexBuffer2> &vertex_buffer,
                                 std::shared_ptr<DescriptorPool> &descriptor_pool,
                                 size_t image_count, SpriteKind kind)
{
    switch (kind) {
        case SpriteKind::VIKING_ROOM:
            return new VikingRoom(device, texture_manager, pipeline_manager, uniform_buffer_manager, vertex_buffer,
            descriptor_pool, image_count);
        case SpriteKind::BAR_SPRITE:
            return new BarSprite(device, texture_manager, pipeline_manager, uniform_buffer_manager, vertex_buffer,
            descriptor_pool, image_count);
        case SpriteKind::BACK_DROP_SPRITE:
            return new BackDropSprite(device, texture_manager, pipeline_manager, uniform_buffer_manager, vertex_buffer,
            descriptor_pool, image_count);
        case SpriteKind::COVER_ART_SPRITE:
            return new CoverArtSprite(device, texture_manager, pipeline_manager, uniform_buffer_manager, vertex_buffer,
                                  descriptor_pool, image_count);

    }
    throw std::invalid_argument("Invalid sprite kind");
}

class InterFrame {
public:
    explicit InterFrame() = default;
    virtual ~InterFrame() = default;

    virtual void inter_frame() {}
};

class Visual {
public:
    explicit Visual(size_t max_frames_in_flight);
    ~Visual() {
        for (size_t i = 0; i < m_max_frames_in_flight; i++) {
            vkDestroySemaphore(m_device->logical_device_handle(), m_render_finished_semaphores[i], nullptr);
            vkDestroySemaphore(m_device->logical_device_handle(), m_image_available_semaphores[i], nullptr);
            vkDestroyFence(m_device->logical_device_handle(), m_in_flight_fences[i], nullptr);
        }

        vkDestroyCommandPool(m_device->logical_device_handle(), m_command_pool, nullptr);

        // vkDestroyDevice(device, nullptr);
        m_device->destroy();
        // delete m_device;

        if (enable_validation_layers) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
                m_instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(m_instance, m_debug_messenger, nullptr);
            }
        }

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyInstance(m_instance, nullptr);

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    void run(InterFrame* inter_frame);
    void draw_frame();

    void load_sprites(std::map<const char*, SpriteKind> &sprites);

    [[nodiscard]] size_t get_image_count() const { return m_max_frames_in_flight; }
    [[nodiscard]] size_t current_frame() const { return m_current_frame; }
    [[nodiscard]] Sprite3* get_sprite(const char* sprite_name) { return m_sprites[sprite_name]; }
    [[nodiscard]] std::shared_ptr<Camera> get_camera() const { return m_uniform_buffer_manager->acquire_camera(); }
private:
    size_t m_max_frames_in_flight;
    GLFWwindow* m_window;
    bool m_window_resized = false;
    bool m_framebuffer_resized = false;
    size_t m_current_frame = 0;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    std::shared_ptr<Device::Device> m_device;
    std::shared_ptr<RenderTarget::RenderTarget> m_render_target;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_render_buffers;

    std::vector<VkSemaphore> m_image_available_semaphores;
    std::vector<VkSemaphore> m_render_finished_semaphores;
    std::vector<VkFence> m_in_flight_fences;

    std::map<const char*, Sprite3*> m_sprites;
    std::shared_ptr<TextureManager> m_texture_manager;
    std::shared_ptr<DescriptorLayoutManager> m_descriptor_layout_manager;
    std::shared_ptr<PipelineLayoutManager> m_pipeline_layout_manager;
    std::shared_ptr<PipelineManager> m_pipeline_manager;
    std::shared_ptr<UniformBufferManager> m_uniform_buffer_manager;
    // std::map<Descriptor2::Kind, std::shared_ptr<DescriptorLayout>> m_descriptor_layout;
    // std::map<Descriptor2::Kind, std::shared_ptr<PipelineLayout2>> m_pipeline_layout;
    // std::map<Pipeline2::Kind, std::shared_ptr<Pipeline2>> m_pipeline;
    // std::map<Texture::Kind, std::shared_ptr<TextureImage2>> m_textures;

    // std::shared_ptr<Camera> m_camera;

    void record_render_buffer(VkCommandBuffer command_buffer, size_t index);
    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
        auto visual = reinterpret_cast<Visual*>(glfwGetWindowUserPointer(window));
        visual->m_window_resized = true;
    }
    static void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
        create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debug_callback;
    }
    static bool check_validation_layer_support() {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char* layer_name : validation_layers) {
            bool layer_found = false;

            for (const auto& layerProperties : available_layers) {
                if (strcmp(layer_name, layerProperties.layerName) == 0) {
                    std::cout << layer_name << " found!" << std::endl;
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found) {
                std::cout << layer_name << " not found!" << std::endl;
                return false;
            }
        }

        return true;
    }
    static std::vector<const char*> get_required_extensions() {
        uint32_t glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        if (enable_validation_layers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    static VkResult create_debug_utils_messenger(VkInstance instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator,
                                                 VkDebugUtilsMessengerEXT *pDebugMessenger) {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    VKAPI_ATTR static VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                         void *pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
    void update_camera_projection() const {
        auto camera = get_camera();
        auto data = camera->get_data();
        const auto width = static_cast<float>(m_render_target->get_swap_chain()->get_extent().width);
        const auto height = static_cast<float>(m_render_target->get_swap_chain()->get_extent().height);
        auto proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 100.0f);
        proj[1][1] *= -1;
        data.proj = proj;
        camera->set_data(data);
    }
};

#endif //VISUAL_H
