//
// Created by Sebastian Sandstig on 2024-12-04.
//

#include "VertexBuffer.h"

#include <Buffer.h>
#include <StagingBuffer.h>

namespace VertexBuffer {
VertexBuffer::VertexBuffer(Spec& spec) : DeviceParent(spec.device) {
    auto command_pool = spec.command_pool;
    auto vertices = spec.vertices;
    auto indices = spec.indices;
    // --------------/
    // Vertex Buffer /
    // ---/----------/
    {
        VkDeviceSize buffer_size = sizeof((*vertices)[0]) * vertices->size();

        StagingBuffer::Spec staging_spec {};
        staging_spec.device = get_device();
        staging_spec.data = vertices->data();
        staging_spec.size = buffer_size;
        auto staging_buffer = new StagingBuffer::StagingBuffer(staging_spec);

        m_vertex_buffer = Buffer::create_buffer(get_device(), buffer_size,
                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        m_vertex_memory = Buffer::bind_buffer(get_device(), m_vertex_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Buffer::copy_buffer(get_device(), command_pool, staging_buffer->get_handle(), m_vertex_buffer, buffer_size);
        delete staging_buffer;
    }
    // ---/

    // --------------/
    // Index Buffer /
    // ---/----------/
    {
        VkDeviceSize buffer_size = sizeof((*indices)[0]) * indices->size();

        StagingBuffer::Spec staging_spec {};
        staging_spec.device = get_device();
        staging_spec.data = indices->data();
        staging_spec.size = buffer_size;
        auto staging_buffer = new StagingBuffer::StagingBuffer(staging_spec);

        m_index_buffer = Buffer::create_buffer(get_device(), buffer_size,
                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        m_index_memory = Buffer::bind_buffer(get_device(), m_index_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        Buffer::copy_buffer(get_device(), command_pool, staging_buffer->get_handle(), m_index_buffer, buffer_size);
        delete staging_buffer;
    }
    // ---/
}

VertexBuffer::~VertexBuffer() {
    auto device = get_device();
    vkDestroyBuffer(device->logical_device_handle(), m_index_buffer, nullptr);
    vkFreeMemory(device->logical_device_handle(), m_index_memory, nullptr);

    vkDestroyBuffer(device->logical_device_handle(), m_vertex_buffer, nullptr);
    vkFreeMemory(device->logical_device_handle(), m_vertex_memory, nullptr);
}


} // VertexBuffer