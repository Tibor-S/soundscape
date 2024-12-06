//
// Created by Sebastian Sandstig on 2024-12-04.
//

#include <Buffer.h>
#include <UniformBuffer.h>

namespace UniformBuffer {

// template<typename T>
// UniformBuffer<T>::UniformBuffer(Spec& spec) : DeviceParent(spec.device) {
//     m_buffer = Buffer::create_buffer(get_device(), data_size(), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
//     m_memory = Buffer::bind_buffer(get_device(), m_buffer,
//                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//     vkMapMemory(get_device()->logical_device_handle(), m_memory, 0, data_size(), 0, &m_data_mapped);
// }

// template<typename T>
// UniformBuffer<T>::~UniformBuffer() {
//     vkDestroyBuffer(get_device()->logical_device_handle(), m_buffer, nullptr);
//     vkFreeMemory(get_device()->logical_device_handle(), m_memory, nullptr);
// }
//
// template<typename T>
// void UniformBuffer<T>::update(T* data) const {
//     memcpy(m_data_mapped, data, sizeof(T));
// }

} // Buffer