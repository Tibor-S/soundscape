//
// Created by Sebastian Sandstig on 2024-12-04.
//

#include "VertexBuffer.h"

#include <Buffer.h>
#include <Model.h>
#include <StagingBuffer.h>
#include <tiny_obj_loader.h>

namespace VertexBuffer {
VertexBuffer::VertexBuffer(Spec& spec) : DeviceParent(spec.device) {
    auto command_pool = spec.command_pool;

    // std::vector<LoadModel*> models;
    // models.resize(models.size());
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    for (auto model_path : spec.models) {
        auto model = new LoadModel(model_path);

        size_t vertex_offset = vertices.size();
        size_t index_offset = indices.size();
        vertices.insert(vertices.end(), model->get_vertices()->begin(), model->get_vertices()->end());
        indices.insert(indices.end(), model->get_indices()->begin(), model->get_indices()->end());
        m_vertex_offset.push_back(vertex_offset);
        m_index_offset.push_back(index_offset);
        m_index_count.push_back(model->get_indices()->size());

        delete model;
    }

    // --------------/
    // Vertex Buffer /
    // ---/----------/
    {
        VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

        StagingBuffer::Spec staging_spec {};
        staging_spec.device = get_device();
        staging_spec.data = vertices.data();
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
        VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

        StagingBuffer::Spec staging_spec {};
        staging_spec.device = get_device();
        staging_spec.data = indices.data();
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