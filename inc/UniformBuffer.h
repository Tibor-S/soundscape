//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H
#include <Device.h>
#include <Buffer.h>

namespace UniformBuffer {

struct Spec {
    Device::Device* device;
};
template<typename T>
class UniformBuffer : public Device::DeviceParent {
public:
    // struct Data {};

    explicit UniformBuffer(Spec& spec) : DeviceParent(spec.device) {
        m_buffer = Buffer::create_buffer(get_device(), sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        m_memory = Buffer::bind_buffer(get_device(), m_buffer,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkMapMemory(get_device()->logical_device_handle(), m_memory, 0, sizeof(T), 0, &m_data_mapped);
    }
    ~UniformBuffer() {
        vkUnmapMemory(get_device()->logical_device_handle(), m_memory);
        vkDestroyBuffer(get_device()->logical_device_handle(), m_buffer, nullptr);
        vkFreeMemory(get_device()->logical_device_handle(), m_memory, nullptr);
    }

    void update(T* data) const {
        memcpy(m_data_mapped, data, sizeof(T));
    }

    [[nodiscard]] VkBuffer get_handle() const { return m_buffer; }
    [[nodiscard]] VkDeviceMemory get_memory_handle() const { return m_memory; }
private:
    void* m_data_mapped = nullptr;
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;

};

template<typename T>
class UniformBufferParent {
public:
    explicit UniformBufferParent(UniformBuffer<T> * uniform_buffer) {m_uniform_buffer = uniform_buffer;}
    UniformBuffer<T>* get_uniform_buffer() const {return m_uniform_buffer;}
private:
    UniformBuffer<T>* m_uniform_buffer;
};


template<typename T>
class UniformBufferParents {
public:
    explicit UniformBufferParents(std::vector<UniformBuffer<T>*>* uniform_buffers) {
        m_uniform_buffers.resize(uniform_buffers->size());
        for (size_t i = 0; i < uniform_buffers->size(); ++i) {
            m_uniform_buffers[i] = (*uniform_buffers)[i];
        }

    }
    UniformBuffer<T>* get_uniform_buffer(size_t i) const { return m_uniform_buffers[i]; }
private:
    std::vector<UniformBuffer<T>*> m_uniform_buffers;
};

} // Buffer

#endif //UNIFORMBUFFER_H
