//
// Created by Sebastian Sandstig on 2024-12-04.
//

#include <StagingBuffer.h>
#include <stb_image.h>
#include <TextureImage.h>

namespace TextureImage {
TextureImage::TextureImage(Spec& spec) : SamplerImage(get_sampler_spec(spec)) {
    auto src_image = spec.src_image;
    auto command_pool = spec.command_pool;

    StagingBufferSpec staging_buffer_spec = {};
    staging_buffer_spec.device = get_device();
    staging_buffer_spec.data = src_image->get_pixels();
    staging_buffer_spec.size = src_image->get_image_size();
    auto staging_buffer = new StagingBuffer(staging_buffer_spec);

    transition_layout(command_pool, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(command_pool, staging_buffer->get_handle());
    generate_mipmaps(command_pool);

    delete staging_buffer;
}

TextureImage::~TextureImage() = default;


} // TextureImage