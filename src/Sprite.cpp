//
// Created by Sebastian Sandstig on 2024-12-04.
//

#include <Sprite.h>

namespace Sprite {

// template<typename T>
// Sprite<T>::Sprite(Spec &spec) : VertexBufferParent(spec.vertex_buffer), TextureImageParent(spec.texture_image),
//                                    DescriptorManagerParent(spec.descriptor_manager)
// {
//     m_uniform_buffer_count = spec.uniform_buffer_count;
//     auto device = get_descriptor_manager()->get_device();
//     UniformBuffer::Spec uniform_buffer_spec = {};
//     uniform_buffer_spec.device = device;
//     m_uniform_buffers.resize(spec.uniform_buffer_count);
//     for (int i = 0; i < spec.uniform_buffer_count; i++) {
//         m_uniform_buffers[i] = new UniformBuffer::UniformBuffer<T>(uniform_buffer_spec);
//     }
//     m_descriptor_indices = get_descriptor_manager()->get_unique_descriptors(m_uniform_buffer_count);
//     auto updater = get_descriptor_manager()->get_updater();
//
//     for (size_t i = 0; i < m_uniform_buffer_count; i++) {
//         updater->update_buffer(m_descriptor_indices[i],
//             0,
//             m_uniform_buffers[i]->get_handle(),
//             0,
//             sizeof(T)
//         );
//         updater->update_image(m_descriptor_indices[i],
//             1,
//             get_texture()->get_image_view(),
//             get_texture()->get_sampler()
//         );
//     }
//     updater->update();
//     free(updater);
// }
//
// template<typename T>
// Sprite<T>::~Sprite() {
//     for (auto uniform_buffer : m_uniform_buffers) {
//         delete uniform_buffer.buffer;
//     }
// };

}