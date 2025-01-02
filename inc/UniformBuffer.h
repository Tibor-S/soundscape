//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <Buffer.h>
#include <Device.h>

struct UniformBufferSpec {
    Device* device;
    size_t size;
};

class UniformBuffer : public DeviceParent {
public:
    explicit UniformBuffer(const UniformBufferSpec& spec) : DeviceParent(spec.device) {
        m_size = spec.size;
        m_buffer = Buffer::create_buffer(get_device(), spec.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        m_memory = Buffer::bind_buffer(get_device(), m_buffer,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkMapMemory(get_device()->logical_device_handle(), m_memory, 0, spec.size, 0, &m_data_mapped);
    }
    ~UniformBuffer() {
        vkUnmapMemory(get_device()->logical_device_handle(), m_memory);
        vkDestroyBuffer(get_device()->logical_device_handle(), m_buffer, nullptr);
        vkFreeMemory(get_device()->logical_device_handle(), m_memory, nullptr);
    }

    void update(const void* data, const size_t size) const {
        memcpy(m_data_mapped, data, size);
    }

    [[nodiscard]] size_t get_size() const { return m_size; }
    [[nodiscard]] VkBuffer get_handle() const { return m_buffer; }
    [[nodiscard]] VkDeviceMemory get_memory_handle() const { return m_memory; }
private:
    void* m_data_mapped = nullptr;
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
    size_t m_size;

};

class UniformBufferParent {
public:
    explicit UniformBufferParent(UniformBuffer* uniform_buffer) {m_uniform_buffer = uniform_buffer;}
    UniformBuffer* get_uniform_buffer() const {return m_uniform_buffer;}
private:
    UniformBuffer* m_uniform_buffer;
};


class UniformBufferParents {
public:
    explicit UniformBufferParents(const std::vector<UniformBuffer*>* uniform_buffers) {
        m_uniform_buffers.resize(uniform_buffers->size());
        for (size_t i = 0; i < uniform_buffers->size(); ++i) {
            m_uniform_buffers[i] = (*uniform_buffers)[i];
        }

    }
    [[nodiscard]] UniformBuffer* get_uniform_buffer(size_t i) const { return m_uniform_buffers[i]; }
private:
    std::vector<UniformBuffer*> m_uniform_buffers;
};

class Camera : public UniformBuffer {
public:
    struct Data {
        glm::mat4 view;
        glm::mat4 proj;
    };

    explicit Camera(const std::shared_ptr<Device>& device) : UniformBuffer(UniformBufferSpec{
                                                                             .device = device.get(),
                                                                             .size = sizeof(Data),
                                                                         }), m_device(device) {}
    ~Camera() = default;

    [[nodiscard]] Data get_data() const { return m_data; }

    void set_data(const Data &data) {
        m_data = data;
        update(&m_data, sizeof(Data));
    }
private:
    std::shared_ptr<Device> m_device;
    Data m_data = {};
};

class UniformBufferManager {
public:
    enum Kind {
        LOCAL,
        CAMERA,
    };

    explicit UniformBufferManager(const std::shared_ptr<Device>& device) : m_device(device) {}

    [[nodiscard]] std::shared_ptr<UniformBuffer> acquire_buffer(const Kind kind) {
        switch (kind) {
            case CAMERA:
                return acquire_camera();
            case LOCAL:
                throw std::invalid_argument("LOCAL Buffers are not handled by manager");
            default:
                throw std::invalid_argument("Kind not implemented");
        }
    }

    void create_camera() {
        m_camera = std::make_shared<Camera>(m_device);
    }
    std::shared_ptr<Camera> acquire_camera() {
        if (!m_camera.has_value()) {
            create_camera();
        }
        return m_camera.value();
    }

private:
    std::shared_ptr<Device> m_device;
    std::optional<std::shared_ptr<Camera>> m_camera = {};
};

#endif //UNIFORMBUFFER_H
