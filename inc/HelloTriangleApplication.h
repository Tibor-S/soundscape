//
// Created by Sebastian Sandstig on 2024-11-25.
//

#ifndef HELLOTRIANGLEAPPLICATION_H
#define HELLOTRIANGLEAPPLICATION_H
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp>

#include <DescriptorManager.h>
// #include <Device.h>
#include <Framebuffer.h>
#include <Memory.h>
#include <Queue.h>
#include <Globals.h>
#include <Pipeline.h>
#include <PipelineLayout.h>
#include <RenderPass.h>
#include <UniformBuffer.h>
#include <RenderTarget.h>
#include <SamplerImage.h>
#include <Sprite.h>
#include <Vertex.h>
#include <VertexBuffer.h>

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class HelloTriangleApplication {
public:
    HelloTriangleApplication();
    void run();

private:
    GLFWwindow* window;
    VkSurfaceKHR surface;

    uint32_t currentFrame = 0;
    uint32_t mipLevels;

    Device::Device* m_device;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    RenderTarget::RenderTarget* m_render_target;

    std::vector<size_t> m_descriptor_indices;
    Descriptor::DescriptorManager * m_descriptor_manager;

    TextureImage::TextureImage* m_texture;

    PipelineLayout::PipelineLayout* m_pipeline_layout;
    Pipeline::Pipeline* m_pipeline;

    // std::vector<UniformBuffer::UniformBuffer<UniformBufferObject>*> m_uniform_buffers;
    Sprite::Sprite* m_sprite;
    Sprite::Sprite* m_sprite2;

    // Sync
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    bool framebufferResized = false;

    // old
    VertexBuffer::VertexBuffer* m_vertex_buffer;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    // Creation
    void createInstance();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void createSurface();
    void createCommandPool();
    void createCommandBuffer();
    void createDescriptorPool();
    // void createDescriptorSets();

    // Record

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    // Present

    void drawFrame();
    void createSyncObjects();
    void updateUniformBuffer(uint32_t currentImage);

    // Validation
    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    // Helper

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);
    // void loadModel();
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    // VkSampleCountFlagBits getMaxUsableSampleCount();

    // Callbacks

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
static std::vector<char> readFile(const std::string& filename);

#endif //HELLOTRIANGLEAPPLICATION_H
