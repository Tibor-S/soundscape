//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef STAGINGBUFFER_H
#define STAGINGBUFFER_H

#include <Device.h>

struct StagingBufferSpec {
    Device* device;
    void* data;
    size_t size;
};

class StagingBuffer : DeviceParent {
public:
    explicit StagingBuffer(const StagingBufferSpec& spec);
    ~StagingBuffer();

    [[nodiscard]] VkBuffer get_handle() const { return m_buffer; }
    [[nodiscard]] VkDeviceMemory get_memory_handle() const { return m_memory; }
    [[nodiscard]] size_t get_max_size() const { return m_max_size; }
private:
    void* m_data_mapped = nullptr;
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
    size_t m_max_size;
};

#endif //STAGINGBUFFER_H
