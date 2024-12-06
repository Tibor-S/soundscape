//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H
#include <Device.h>
#include <Vertex.h>

namespace VertexBuffer {

struct Spec {
    Device::Device* device;
    VkCommandPool command_pool;
    std::vector<Vertex>* vertices;
    std::vector<uint32_t>* indices;
};

class VertexBuffer : public Device::DeviceParent {
public:
    VertexBuffer(Spec& spec);
    ~VertexBuffer();

    [[nodiscard]] VkBuffer get_vertex_handle() const { return m_vertex_buffer; }
    [[nodiscard]] VkDeviceMemory get_vertex_memory_handle() const { return m_vertex_memory; }
    [[nodiscard]] VkBuffer get_index_handle() const { return m_index_buffer; }
    [[nodiscard]] VkDeviceMemory get_index_memory_handle() const { return m_index_memory; }
private:
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

#endif //VERTEXBUFFER_H
