//
// Created by Sebastian Sandstig on 2024-12-04.
//

#include <StagingBuffer.h>
#include <Buffer.h>

StagingBuffer::StagingBuffer(const StagingBufferSpec& spec) : DeviceParent(spec.device) {
    m_max_size = spec.size;
    m_buffer = Buffer::create_buffer(get_device(), m_max_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    m_memory = Buffer::bind_buffer(get_device(), m_buffer,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkMapMemory(get_device()->logical_device_handle(), m_memory, 0, m_max_size, 0, &m_data_mapped);
    memcpy(m_data_mapped, spec.data, m_max_size);
}

StagingBuffer::~StagingBuffer() {
    // vkUnmapMemory(get_device()->logical_device_handle(), m_memory);
    vkDestroyBuffer(get_device()->logical_device_handle(), m_buffer, nullptr);
    vkFreeMemory(get_device()->logical_device_handle(), m_memory, nullptr);
}
