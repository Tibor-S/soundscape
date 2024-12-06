//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef SPRITE_H
#define SPRITE_H
#include <DescriptorManager.h>
#include <TextureImage.h>
#include <UniformBuffer.h>
#include <VertexBuffer.h>

namespace Sprite {

struct Spec {
    VertexBuffer::VertexBuffer* vertex_buffer;
    TextureImage::TextureImage* texture_image;
    Descriptor::DescriptorManager* descriptor_manager;
    size_t uniform_buffer_count;
};

template<typename T>
class Sprite : public VertexBuffer::VertexBufferParent,
               public TextureImage::TextureImageParent,
               public Descriptor::DescriptorManagerParent {
public:
    Sprite(Spec &spec) : VertexBufferParent(spec.vertex_buffer), TextureImageParent(spec.texture_image),
                         DescriptorManagerParent(spec.descriptor_manager)
    {
        m_uniform_buffer_count = spec.uniform_buffer_count;
        auto device = get_descriptor_manager()->get_device();
        UniformBuffer::Spec uniform_buffer_spec = {};
        uniform_buffer_spec.device = device;
        m_uniform_buffers.resize(spec.uniform_buffer_count);
        for (int i = 0; i < spec.uniform_buffer_count; i++) {
            m_uniform_buffers[i] = new UniformBuffer::UniformBuffer<T>(uniform_buffer_spec);
        }
        m_descriptor_indices = get_descriptor_manager()->get_unique_descriptors(m_uniform_buffer_count);
        auto updater = get_descriptor_manager()->get_updater();

        for (size_t i = 0; i < m_uniform_buffer_count; i++) {
            auto uniform_buffer = m_uniform_buffers[i];
            updater->update_buffer(m_descriptor_indices[i],
                0,
                uniform_buffer->get_handle(),
                0,
                sizeof(T)
            );
            updater->update_image(m_descriptor_indices[i],
                1,
                get_texture()->get_image_view(),
                get_texture()->get_sampler()
            );
        }
        updater->update();
        free(updater);
    };
    ~Sprite() {
        for (auto uniform_buffer : m_uniform_buffers) {
            delete uniform_buffer;
        }
    };

    UniformBuffer::UniformBuffer<T>* get_uniform_buffer(size_t i) { return m_uniform_buffers[i]; }
    size_t get_descriptor(size_t i) { return m_descriptor_indices[i]; }
private:
    size_t m_uniform_buffer_count;
    std::vector<UniformBuffer::UniformBuffer<T>*> m_uniform_buffers;
    std::vector<size_t> m_descriptor_indices;
};

}


#endif //SPRITE_H
