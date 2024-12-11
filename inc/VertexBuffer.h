//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H
#include <Buffer.h>
#include <Device.h>
#include <Model.h>
#include <StagingBuffer.h>
#include <Vertex.h>
#include <__ranges/elements_view.h>
#include <__ranges/views.h>

namespace VertexBuffer {

struct Spec {
    Device::Device* device;
    VkCommandPool command_pool;
    std::vector<const char*> models;
};

class VertexBuffer : public Device::DeviceParent {
public:
    VertexBuffer(Spec& spec);
    ~VertexBuffer();

    [[nodiscard]] VkBuffer get_vertex_handle() const { return m_vertex_buffer; }
    [[nodiscard]] VkDeviceMemory get_vertex_memory_handle() const { return m_vertex_memory; }
    [[nodiscard]] VkBuffer get_index_handle() const { return m_index_buffer; }
    [[nodiscard]] VkDeviceMemory get_index_memory_handle() const { return m_index_memory; }
    [[nodiscard]] size_t get_vertex_offset(size_t index) const { return m_vertex_offset[index]; }
    [[nodiscard]] size_t get_index_offset(size_t index) const { return m_index_offset[index]; }
    [[nodiscard]] size_t get_index_count(size_t index) const { return m_index_count[index]; }
private:
    std::vector<size_t> m_vertex_offset;
    std::vector<size_t> m_index_offset;
    std::vector<size_t> m_index_count;
    VkBuffer m_vertex_buffer;
    VkDeviceMemory m_vertex_memory;
    VkBuffer m_index_buffer;
    VkDeviceMemory m_index_memory;
};

class VertexBufferParent {
public:
    explicit VertexBufferParent(VertexBuffer* vertex_buffer) {m_vertex_buffer = vertex_buffer;}
    [[nodiscard]] VertexBuffer* get_vertex_buffer() const {return m_vertex_buffer;}
private:
    VertexBuffer* m_vertex_buffer;
};

} // VertexBuffer

class VertexBuffer2 {
public:
    struct ModelPosition {
        size_t vertex_offset;
        size_t index_offset;
        size_t index_count;
    };

    VertexBuffer2(const std::shared_ptr<Device::Device> &device, VkCommandPool command_pool,
                  const std::vector<Model*> &models) : m_device(device) {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        for (const auto& model : models) {
            m_position[model->get_kind()] = {
                .vertex_offset = vertices.size(),
                .index_offset = indices.size(),
                .index_count = model->get_indices()->size(),
            };

            vertices.insert(vertices.end(), model->get_vertices()->begin(), model->get_vertices()->end());
            indices.insert(indices.end(), model->get_indices()->begin(), model->get_indices()->end());
        }

        {
            VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

            StagingBuffer::Spec staging_spec {};
            staging_spec.device = m_device.get();
            staging_spec.data = vertices.data();
            staging_spec.size = buffer_size;
            auto staging_buffer = new StagingBuffer::StagingBuffer(staging_spec);

            m_vertex_buffer = Buffer::create_buffer(m_device.get(), buffer_size,
                                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            m_vertex_memory = Buffer::bind_buffer(m_device.get(), m_vertex_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            Buffer::copy_buffer(m_device.get(), command_pool, staging_buffer->get_handle(), m_vertex_buffer,
                                buffer_size);
            delete staging_buffer;
        }

        {
            VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

            StagingBuffer::Spec staging_spec {};
            staging_spec.device = m_device.get();
            staging_spec.data = indices.data();
            staging_spec.size = buffer_size;
            auto staging_buffer = new StagingBuffer::StagingBuffer(staging_spec);

            m_index_buffer = Buffer::create_buffer(m_device.get(), buffer_size,
                                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            m_index_memory = Buffer::bind_buffer(m_device.get(), m_index_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            Buffer::copy_buffer(m_device.get(), command_pool, staging_buffer->get_handle(), m_index_buffer,
                                buffer_size);
            delete staging_buffer;
        }
    }

    ~VertexBuffer2() {
        auto device = m_device.get();
        vkDestroyBuffer(device->logical_device_handle(), m_index_buffer, nullptr);
        vkFreeMemory(device->logical_device_handle(), m_index_memory, nullptr);

        vkDestroyBuffer(device->logical_device_handle(), m_vertex_buffer, nullptr);
        vkFreeMemory(device->logical_device_handle(), m_vertex_memory, nullptr);
    }

    [[nodiscard]] VkBuffer get_vertex_buffer() const { return m_vertex_buffer; }
    [[nodiscard]] VkBuffer get_index_buffer() const { return m_index_buffer; }
    [[nodiscard]] ModelPosition get_model_position(Model::Kind kind) const { return m_position.at(kind); }
private:
    std::shared_ptr<Device::Device> m_device;

    std::map<Model::Kind, ModelPosition> m_position;
    VkBuffer m_vertex_buffer;
    VkDeviceMemory m_vertex_memory;
    VkBuffer m_index_buffer;
    VkDeviceMemory m_index_memory;
};

#endif //VERTEXBUFFER_H
