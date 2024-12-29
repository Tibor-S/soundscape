//
// Created by Sebastian Sandstig on 2024-12-06.
//

#include "Visual.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

Visual::Visual(size_t max_frames_in_flight) : m_max_frames_in_flight(max_frames_in_flight) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);

    // Instance
    {
        if (enable_validation_layers && !check_validation_layer_support()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = get_required_extensions();
        extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enable_validation_layers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            createInfo.ppEnabledLayerNames = validation_layers.data();

            populate_debug_messenger_create_info(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    // DebugMessenger
    {
        if (!enable_validation_layers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populate_debug_messenger_create_info(createInfo);

        if (create_debug_utils_messenger(m_instance, &createInfo, nullptr, &m_debug_messenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    // Surface
    {
        if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    // Device
    {
        Device::Spec device_spec = {
            .instance = m_instance
        };
        m_device = std::make_shared<Device::Device>(device_spec, m_surface);
    }

    // Render Target
    {
        RenderTarget::Spec spec {};
        spec.max_image_count = m_max_frames_in_flight + 1;
        spec.device = m_device.get();
        spec.surface_handle = m_surface;
        spec.window = m_window;
        m_render_target = std::make_shared<RenderTarget::RenderTarget>(spec);
    }

    // Command Pool
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_device->queue_index(Queue::GRAPHICS).value();

        if (vkCreateCommandPool(m_device->logical_device_handle(), &poolInfo, nullptr, &m_command_pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    // Render Buffer
    {
        m_render_buffers.resize(m_max_frames_in_flight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_command_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_render_buffers.size());

        if (vkAllocateCommandBuffers(m_device->logical_device_handle(), &allocInfo, m_render_buffers.data()) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    // Sync Objects
    {
        m_image_available_semaphores.resize(m_max_frames_in_flight);
        m_render_finished_semaphores.resize(m_max_frames_in_flight);
        m_in_flight_fences.resize(m_max_frames_in_flight);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < m_max_frames_in_flight; i++) {
            if (vkCreateSemaphore(m_device->logical_device_handle(), &semaphore_info, nullptr,
                                 &m_image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device->logical_device_handle(), &semaphore_info, nullptr,
                                 &m_render_finished_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device->logical_device_handle(), &fence_info, nullptr, &m_in_flight_fences[i])
                    != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    // Managers
    m_descriptor_layout_manager =  std::make_shared<DescriptorLayoutManager>(m_device, m_command_pool);
    m_pipeline_layout_manager = std::make_shared<PipelineLayoutManager>(m_device, m_descriptor_layout_manager,
                                                                        m_command_pool);
    m_pipeline_manager = std::make_shared<PipelineManager>(m_device, m_pipeline_layout_manager, m_render_target,
                                                           m_command_pool);
    m_texture_manager = std::make_shared<TextureManager>(m_device, m_command_pool);
    m_uniform_buffer_manager = std::make_shared<UniformBufferManager>(m_device);

    // Camera
    // m_camera = std::make_shared<Camera>(m_device);
    auto camera = m_uniform_buffer_manager->acquire_camera();
    const auto width = static_cast<float>(m_render_target->get_swap_chain()->get_extent().width);
    const auto height = static_cast<float>(m_render_target->get_swap_chain()->get_extent().height);
    auto proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 1000.0f);
    proj[1][1] *= -1;
    camera->set_data({
        .view = glm::lookAt(glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .proj = proj,
    });

}

void Visual::run(InterFrame* inter_frame) {
    // size_t i = 0;
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        inter_frame->inter_frame();
        draw_frame();
    }

    vkDeviceWaitIdle(m_device->logical_device_handle());
}

void Visual::load_sprites(std::map<const char*, SpriteKind> &sprites) {
    std::map<Descriptor2::Kind, std::shared_ptr<DescriptorPool>> unique_descriptor_pools;
    // std::map<Pipeline2::Kind, std::shared_ptr<Pipeline2>> unique_pipelines;
    // std::map<Texture::Kind, std::shared_ptr<TextureImage2>> unique_textures;
    std::unordered_set<Model::Kind> unique_models;
    std::map<Descriptor2::Kind, size_t> descriptor_count;

    for (const auto& [id, kind] : sprites) {
        const auto descriptor_kind = get_descriptor_kind(kind).value();
        // const auto pipeline_kind = get_pipeline_kind(kind).value();
        const auto model_kind = get_model_kind(kind).value();
        // const auto texture_kind = get_texture_kind(kind).value();
        // assert (descriptor_kind == Pipeline2::get_layout_kind(pipeline_kind));

        // Descriptor Pool
        // if (!m_descriptor_layout.contains(descriptor_kind)) {
        //     const auto layout = std::make_shared<DescriptorLayout>(m_device, descriptor_kind);
        //     m_descriptor_layout[descriptor_kind] = layout;
        // }
        // auto descriptor_layout = m_descriptor_layout[descriptor_kind];
        if (!descriptor_count.contains(descriptor_kind)) {
            descriptor_count[descriptor_kind] = m_max_frames_in_flight;
        } else {
            descriptor_count[descriptor_kind] += m_max_frames_in_flight;
        }

        // Pipeline
        // if (!m_pipeline_layout.contains(descriptor_kind)) {
        //     const auto layout = std::make_shared<PipelineLayout2>(m_device, descriptor_layout);
        //     m_pipeline_layout[descriptor_kind] = layout;
        // }
        // auto pipeline_layout = m_pipeline_layout[descriptor_kind];
        // if (!unique_pipelines.contains(pipeline_kind)) {
        //     const auto pipeline = std::make_shared<Pipeline2>(m_device, pipeline_layout, m_render_target,
        //                                                       pipeline_kind);
        //     unique_pipelines[pipeline_kind] = pipeline;
        // }

        // Textures
        // if (!m_textures.contains(texture_kind)) {
        //     const auto texture = new Texture(texture_kind);
        //     const auto texture_image = std::make_shared<TextureImage2>(m_device, m_command_pool, texture);
        //     delete texture;
        //     m_textures[texture_kind] = texture_image;
        // }
        // if (!unique_textures.contains(texture_kind)) {
        //     unique_textures[texture_kind] = m_textures[texture_kind];
        // }

        // Models
        if (!unique_models.contains(model_kind)) {
            unique_models.insert(model_kind);
        }
    }

    for (const auto& [kind, count] : descriptor_count) {
        if (!unique_descriptor_pools.contains(kind)) {
            auto layout = m_descriptor_layout_manager->acquire_descriptor_layout(kind);
            const auto descriptor_pool = std::make_shared<DescriptorPool>(
                m_device, layout, kind, count);
            unique_descriptor_pools[kind] = descriptor_pool;
        }
    }

    // StandardVertex Buffer
    std::vector<Model*> loaded_models = {};
    loaded_models.reserve(unique_models.size());
    for (const auto kind : unique_models) {
        auto model = new Model(kind);
        loaded_models.push_back(model);
    }
    auto vertex_buffer = std::make_shared<VertexBuffer2>(m_device, m_command_pool, loaded_models);
    for (const auto model : loaded_models) {
        delete model;
    }

    for (const auto& [id, kind] : sprites) {
        const auto descriptor_kind = get_descriptor_kind(kind);
        // const auto pipeline_kind = Sprite2::get_kind_pipeline(kind);
        const auto model_kind = get_model_kind(kind);
        // const auto texture_kind = Sprite2::get_kind_texture(kind);
        // assert (descriptor_kind == Pipeline2::get_layout_kind(pipeline_kind));

        auto pool = unique_descriptor_pools[descriptor_kind.value()];
        // auto pipeline = unique_pipelines[pipeline_kind];
        // auto texture = unique_textures[texture_kind];


        auto sprite = construct_sprite(m_device, *m_texture_manager, *m_pipeline_manager, *m_uniform_buffer_manager,
                                       vertex_buffer, pool, m_max_frames_in_flight, kind);
        // auto sprite = new Sprite2(vertex_buffer, texture, pipeline, pool, kind, m_max_frames_in_flight);
        // for (size_t i = 0; i < m_max_frames_in_flight; i++) {
        //     auto upd = sprite->get_descriptor_set_updater(i);
        //     // upd->update_buffer(0, *m_camera);
        //
        //     // upd->update_buffer(1, *m_camera);
        // }


        m_sprites[id] = sprite;
    }
}


void Visual::draw_frame() {
    vkWaitForFences(m_device->logical_device_handle(), 1, &m_in_flight_fences[m_current_frame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_device->logical_device_handle(),
                                            m_render_target->get_swap_chain()->get_handle(), UINT64_MAX,
                                            m_image_available_semaphores[m_current_frame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_render_target->recreate_swap_chain();
        update_camera_projection();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // updateUniformBuffer(currentFrame);

    vkResetFences(m_device->logical_device_handle(), 1, &m_in_flight_fences[m_current_frame]);

    vkResetCommandBuffer(m_render_buffers[m_current_frame], /*VkCommandBufferResetFlagBits*/ 0);
    record_render_buffer(m_render_buffers[m_current_frame], imageIndex);
    // recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_image_available_semaphores[m_current_frame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_render_buffers[m_current_frame];

    VkSemaphore signalSemaphores[] = {m_render_finished_semaphores[m_current_frame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkQueue graphics_queue = m_device->queue(Queue::GRAPHICS).value();
    if (vkQueueSubmit(graphics_queue, 1, &submitInfo, m_in_flight_fences[m_current_frame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_render_target->get_swap_chain()->get_handle()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    VkQueue present_queue = m_device->queue(Queue::PRESENTATION).value();
    result = vkQueuePresentKHR(present_queue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized) {
        m_framebuffer_resized = false;
        m_render_target->recreate_swap_chain();
        update_camera_projection();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_current_frame = (m_current_frame + 1) % m_max_frames_in_flight;
}

void Visual::record_render_buffer(VkCommandBuffer command_buffer, size_t index) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_render_target->get_render_pass()->get_render_pass_handle();
    renderPassInfo.framebuffer = m_render_target->get_framebuffer(index)->get_framebuffer_handle();
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_render_target->get_swap_chain()->get_extent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_render_target->get_swap_chain()->get_extent().width);
    viewport.height = static_cast<float>(m_render_target->get_swap_chain()->get_extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_render_target->get_swap_chain()->get_extent();
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    for (const auto [_, sprite] : m_sprites) {
        // sprite
        // auto upd = sprite->get_descriptor_set_updater(m_current_frame);
        // model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        // upd->update_buffer(1, )

        auto pipeline_kind = sprite->pipeline_kind();
        auto pipeline = m_pipeline_manager->acquire_pipeline(pipeline_kind.value());
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_handle());

        auto vertex_position = sprite->get_vertex_position();
        auto vertex_buffer = sprite->get_vertex_buffer();
        VkBuffer vertexBuffers[] = {vertex_buffer->get_vertex_buffer()};
        VkDeviceSize offsets[] = {vertex_position.vertex_offset};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, vertex_buffer->get_index_buffer(), 0, VK_INDEX_TYPE_UINT32);

        auto descriptor = sprite->get_descriptor_set(m_current_frame);
        auto descriptor_set = descriptor->get_descriptor_set();
        auto pipeline_layout = m_pipeline_layout_manager->acquire_pipeline_layout(descriptor->get_kind()); // m_pipeline_layout[descriptor->get_kind()];
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout->get_handle(), 0, 1,
                                &descriptor_set, 0, nullptr);

        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(vertex_position.index_count), 1,
                         vertex_position.index_offset, /* static_cast<int32_t>(vertex_position.vertex_offset)*/0, 0);
    }

    // auto vertex_buffer = m_sprite->get_vertex_buffer();
    // // auto uniform_buffer = m_sprite->get_uniform_buffer(currentFrame);
    // VkBuffer vertexBuffers[] = {vertex_buffer->get_vertex_handle()};
    // VkDeviceSize offsets[] = {0};
    // vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    //
    // vkCmdBindIndexBuffer(commandBuffer, vertex_buffer->get_index_handle(), 0, VK_INDEX_TYPE_UINT32);
    //
    // auto descriptor_index = m_sprite->get_descriptor(currentFrame);//m_descriptor_indices[currentFrame];
    // auto descriptor = m_descriptor_manager->descriptor_handle(descriptor_index);
    // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout->get_handle(), 0, 1,
    //                         &descriptor, 0, nullptr);
    //
    // vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_vertex_buffer->get_index_count(0)), 1, 0, 0, 0);
    // m_sprite->draw(commandBuffer, currentFrame);
    // m_sprite2->draw(commandBuffer, currentFrame);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}
