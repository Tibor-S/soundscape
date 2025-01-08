//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef SPRITE_H
#define SPRITE_H

#include <Descriptor.h>
#include <Model.h>
#include <Pipeline.h>
#include <Texture.h>
#include <UniformBuffer.h>
#include <VertexBuffer.h>

class Sprite {
public:
    Sprite(const Pipeline::Kind pipeline_kind, const Model::Kind model_kind, PipelineManager &pipeline_manager,
           const std::shared_ptr<VertexBuffer> &vertex_buffer, std::shared_ptr<DescriptorPool> &descriptor_pool,
           const size_t image_count) : m_vertex_buffer(vertex_buffer)
    {
        m_pipeline = pipeline_manager.acquire_pipeline(pipeline_kind);

        m_descriptor_sets = DescriptorSet::create_descriptor_sets(descriptor_pool, image_count);
        m_vertex_buffer_position = m_vertex_buffer->get_model_position(model_kind);
    }

    void set_descriptor_sets(const std::shared_ptr<Device>& device, TextureManager &texture_manager,
                             UniformBufferManager &uniform_buffer_manager, const size_t image_count) {
        for (size_t i = 0; i < image_count; i++) {
            auto descriptor_set = get_descriptor_set(i);
            const auto updater = new DescriptorSetUpdater(descriptor_set);
            std::vector<std::shared_ptr<UniformBuffer>> uniform_buffers;
            std::vector<std::shared_ptr<TextureImage2>> textures;

            for (const auto binding : m_buffer_bindings) {
                auto kind = binding_buffer_kind(binding);
                if (kind == UniformBufferManager::LOCAL) {
                    const auto size = binding_buffer_size(binding);
                    if (!size) {
                        throw std::runtime_error("Buffer size is not set for binding");
                    }
                    UniformBufferSpec spec = {
                        .device = device.get(),
                        .size = size,
                    };
                    auto buffer = std::make_shared<UniformBuffer>(spec);
                    uniform_buffers.push_back(buffer);
                    updater->update_buffer(binding, *buffer);
                } else {
                    auto buffer = uniform_buffer_manager.acquire_buffer(kind);
                    uniform_buffers.push_back(buffer);
                    updater->update_buffer(binding, *buffer);
                }
            }


            for (const auto binding : m_image_bindings) {
                auto kind = binding_image_kind(binding);
                if (kind == Texture::LOCAL) {
                    // TODO: Make it possible to change this
                    const auto texture = new Texture(Texture::LOCAL, 400, 400, {255, 255, 255, 255});
                    const auto texture_image = texture_manager.create_local_texture(texture);
                    delete texture;
                    textures.push_back(texture_image);
                    updater->update_image(binding, *texture_image);
                } else {
                    auto image = texture_manager.acquire_texture(kind);
                    textures.push_back(image);
                    updater->update_image(binding, *image);
                }
            }

            updater->finalize();

            delete updater;

            m_buffers.push_back(uniform_buffers);
            m_textures.push_back(textures);
        }
    }

    [[nodiscard]] UniformBufferManager::Kind binding_buffer_kind(const size_t binding) const {
        return m_binding_buffer_kind[get_buffer_binding_index(binding)];
    }
    [[nodiscard]] size_t binding_buffer_size(const size_t binding) const {
        return m_binding_buffer_size[get_buffer_binding_index(binding)];
    }
    [[nodiscard]] Texture::Kind binding_image_kind(const size_t binding) const {
        return m_binding_image_kind[get_image_binding_index(binding)];
    }
    [[nodiscard]] std::optional<Model::Kind> model_kind() { return m_model_kind; }
    [[nodiscard]] std::optional<Texture::Kind> texture_kind() { return m_texture_kind; }
    [[nodiscard]] std::optional<Pipeline::Kind> pipeline_kind() { return m_pipeline_kind; }
    [[nodiscard]] std::optional<DescriptorSet::Kind> descriptor_set_kind() { return m_descriptor_set_kind; }
    [[nodiscard]] std::shared_ptr<DescriptorSet> get_descriptor_set(const size_t index) const {
        return m_descriptor_sets[index];
    }
    [[nodiscard]] std::shared_ptr<VertexBuffer> get_vertex_buffer() const { return m_vertex_buffer; }
    [[nodiscard]] VertexBuffer::ModelPosition get_vertex_position() const { return m_vertex_buffer_position; }
    [[nodiscard]] std::shared_ptr<UniformBuffer> get_buffer(const size_t image_index, const size_t binding) const {
        return m_buffers[image_index][get_buffer_binding_index(binding)];
    }
    [[nodiscard]] std::shared_ptr<TextureImage2> get_image(const size_t image_index, const size_t binding) const {
        return m_textures[image_index][get_image_binding_index(binding)];
    }

    void set_buffer(const size_t image_index, const size_t binding, const void* data, const size_t size) const {
        auto buffer = get_buffer(image_index, binding);
        buffer->update(data, size);
    }
    void set_image(const size_t image_index, const size_t binding, uint8_t* pixel_data, const size_t size) const {
        auto image = get_image(image_index, binding);
        image->update(pixel_data, size);
    }

    void set_buffer_bindings(const std::vector<size_t>& bindings) {
        m_buffer_bindings.resize(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++)
            m_buffer_bindings[i] = bindings[i];
    }
    void set_image_bindings(const std::vector<size_t>& bindings) {
        m_image_bindings.resize(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++)
            m_image_bindings[i] = bindings[i];
    }
    void set_binding_buffer_kind(const std::vector<UniformBufferManager::Kind>& bindings) {
        m_binding_buffer_kind.resize(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++)
            m_binding_buffer_kind[i] = bindings[i];
    }
    void set_binding_buffer_size(const std::vector<size_t>& bindings) {
        m_binding_buffer_size.resize(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++)
            m_binding_buffer_size[i] = bindings[i];
    }
    void set_binding_image_kind(const std::vector<Texture::Kind>& bindings) {
        m_binding_image_kind.resize(bindings.size());
        for (size_t i = 0; i < bindings.size(); i++)
            m_binding_image_kind[i] = bindings[i];
    }
    void set_model_kind(const Model::Kind kind) { m_model_kind = kind; }
    void set_pipeline_kind(const Pipeline::Kind kind) { m_pipeline_kind = kind; }
    void set_descriptor_set_kind(const DescriptorSet::Kind kind) { m_descriptor_set_kind = kind; }
private:
    std::vector<std::shared_ptr<DescriptorSet>> m_descriptor_sets;
    std::vector<std::vector<std::shared_ptr<TextureImage2>>> m_textures;
    std::vector<std::vector<std::shared_ptr<UniformBuffer>>> m_buffers;
    std::shared_ptr<Pipeline> m_pipeline;
    std::shared_ptr<VertexBuffer> m_vertex_buffer;
    VertexBuffer::ModelPosition m_vertex_buffer_position {};

    std::vector<size_t> m_buffer_bindings;
    std::vector<size_t> m_image_bindings;
    std::vector<UniformBufferManager::Kind> m_binding_buffer_kind;
    std::vector<size_t> m_binding_buffer_size;
    std::vector<Texture::Kind> m_binding_image_kind;

    Model::Kind m_model_kind {};
    Texture::Kind m_texture_kind {};
    Pipeline::Kind m_pipeline_kind {};
    DescriptorSet::Kind m_descriptor_set_kind {};


    [[nodiscard]] size_t get_buffer_binding_index(const size_t binding) const {
        if (const auto index = std::ranges::find(m_buffer_bindings, binding); index != m_buffer_bindings.end()) {
            return index - m_buffer_bindings.begin();
        }
        throw std::runtime_error("No buffer present on binding");
    }
    [[nodiscard]] size_t get_image_binding_index(const size_t binding) const {
        if (const auto index = std::ranges::find(m_image_bindings, binding); index != m_image_bindings.end()) {
            return index - m_image_bindings.begin();
        }
        throw std::runtime_error("No buffer present on binding");
    }
};

struct SpriteModel {
    glm::mat4 model_matrix;
};

class CameraModelSamplerSprite : public Sprite {
public:
    CameraModelSamplerSprite(const Model::Kind model_kind, PipelineManager &pipeline_manager,
                             const std::shared_ptr<VertexBuffer> &vertex_buffer,
                             std::shared_ptr<DescriptorPool> &descriptor_pool,
                             const size_t image_count) : Sprite(Pipeline::STANDARD, model_kind, pipeline_manager,
                                                                vertex_buffer, descriptor_pool, image_count)
    {
        set_buffer_bindings({0, 1});
        set_image_bindings({2});
        set_binding_buffer_kind({
            UniformBufferManager::CAMERA, UniformBufferManager::LOCAL
        });
        set_binding_buffer_size({sizeof(Camera::Data), sizeof(SpriteModel)});
        set_binding_image_kind({Texture::TX_NULL});
        set_pipeline_kind(Pipeline::STANDARD);
        set_descriptor_set_kind(Descriptor::CAMERA_MODEL_SAMPLER);
    }

    [[nodiscard]] static std::optional<Pipeline::Kind> pipeline_kind() { return Pipeline::STANDARD; }
    [[nodiscard]] static std::optional<Descriptor::Kind> descriptor_set_kind() {
        return Descriptor::CAMERA_MODEL_SAMPLER;
    }

    void set_sprite_model(const SpriteModel& sprite_model, size_t image_index) const {
        set_buffer(image_index, 1, &sprite_model, sizeof(SpriteModel));
    }
};


class VikingRoom : public CameraModelSamplerSprite {
public:
    VikingRoom(const std::shared_ptr<Device> &device, TextureManager &texture_manager,
               PipelineManager &pipeline_manager,
               UniformBufferManager &uniform_buffer_manager,
               const std::shared_ptr<VertexBuffer> &vertex_buffer,
               std::shared_ptr<DescriptorPool> &descriptor_pool,
               const size_t image_count) : CameraModelSamplerSprite(Model::VIKING_ROOM, pipeline_manager, vertex_buffer,
                                                                    descriptor_pool, image_count)
    {
        set_binding_image_kind({Texture::VIKING_ROOM});
        set_model_kind(Model::VIKING_ROOM);
        set_descriptor_sets(device, texture_manager, uniform_buffer_manager, image_count);
    }

    [[nodiscard]] static std::optional<Model::Kind> model_kind() { return Model::VIKING_ROOM; }
    [[nodiscard]] static std::optional<Texture::Kind> texture_kind() { return Texture::VIKING_ROOM; }
};


struct BoneBuffer {
    glm::mat4 bone[2];
};


class CameraModelBufferSprite : public Sprite {
public:
    CameraModelBufferSprite(const Model::Kind model_kind, PipelineManager &pipeline_manager,
                            const std::shared_ptr<VertexBuffer> &vertex_buffer,
                            std::shared_ptr<DescriptorPool> &descriptor_pool,
                            const size_t image_count) : Sprite(Pipeline::BAR, model_kind, pipeline_manager,
                                                               vertex_buffer, descriptor_pool, image_count)
    {
        set_buffer_bindings({0, 1, 2});
        set_image_bindings({});
        set_binding_buffer_kind({
            UniformBufferManager::CAMERA, UniformBufferManager::LOCAL, UniformBufferManager::LOCAL
        });
        set_binding_buffer_size({sizeof(Camera::Data), sizeof(SpriteModel), sizeof(BoneBuffer)});
        set_binding_image_kind({Texture::TX_NULL});
        set_pipeline_kind(Pipeline::BAR);
        set_descriptor_set_kind(Descriptor::BAR);
    }

    void set_sprite_model(const SpriteModel& sprite_model, const size_t image_index) const {
        set_buffer(image_index, 1, &sprite_model, sizeof(SpriteModel));
    }

    void set_bone_model(const BoneBuffer& bone_buffer, const size_t image_index) const {
        set_buffer(image_index, 2, &bone_buffer, sizeof(BoneBuffer));
    }
};

class BarSprite : public Sprite {
public:
    BarSprite(const std::shared_ptr<Device> &device, TextureManager &texture_manager,
              PipelineManager &pipeline_manager, UniformBufferManager &uniform_buffer_manager,
              const std::shared_ptr<VertexBuffer> &vertex_buffer, std::shared_ptr<DescriptorPool> &descriptor_pool,
              const size_t image_count) : Sprite(Pipeline::BAR, Model::BAR, pipeline_manager, vertex_buffer,
                                                 descriptor_pool, image_count)
    {

        set_buffer_bindings({0, 1, 2, 3});
        set_image_bindings({});
        set_binding_buffer_kind({UniformBufferManager::CAMERA, UniformBufferManager::LOCAL,
            UniformBufferManager::LOCAL, UniformBufferManager::LOCAL});
        set_binding_buffer_size({sizeof(Camera::Data), sizeof(SpriteModel), sizeof(BoneBuffer), sizeof(glm::vec3)});
        set_binding_image_kind({});

        set_pipeline_kind(Pipeline::BAR);
        set_descriptor_set_kind(Descriptor::BAR);
        set_model_kind(Model::BAR);

        set_descriptor_sets(device, texture_manager, uniform_buffer_manager, image_count);
        //
    }

    [[nodiscard]] static std::optional<Model::Kind> model_kind() { return Model::BAR; }
    [[nodiscard]] static std::optional<Texture::Kind> texture_kind() { return Texture::TX_NULL; }
    [[nodiscard]] static std::optional<Pipeline::Kind> pipeline_kind() { return Pipeline::BAR; }
    [[nodiscard]] static std::optional<Descriptor::Kind> descriptor_set_kind() {
        return Descriptor::BAR;
    }
};

struct CornerColors {
    std::array<glm::vec4, 5> color;
};

class BackDropSprite : public Sprite {
public:
    BackDropSprite(const std::shared_ptr<Device> &device, TextureManager &texture_manager,
                   PipelineManager &pipeline_manager, UniformBufferManager &uniform_buffer_manager,
                   const std::shared_ptr<VertexBuffer> &vertex_buffer, std::shared_ptr<DescriptorPool> &descriptor_pool,
                   const size_t image_count) : Sprite(Pipeline::BACK_DROP, Model::BACK_DROP, pipeline_manager,
                                                      vertex_buffer, descriptor_pool, image_count)
    {
        set_buffer_bindings({0, 1});
        set_image_bindings({});
        set_binding_buffer_kind({
            UniformBufferManager::CAMERA, UniformBufferManager::LOCAL
        });
        set_binding_buffer_size({sizeof(Camera::Data), sizeof(CornerColors)});
        set_binding_image_kind({});

        set_pipeline_kind(Pipeline::BACK_DROP);
        set_descriptor_set_kind(Descriptor::BACK_DROP);
        set_model_kind(Model::BACK_DROP);

        set_descriptor_sets(device, texture_manager, uniform_buffer_manager, image_count);
    }

    [[nodiscard]] static std::optional<Model::Kind> model_kind() { return Model::BACK_DROP; }
    [[nodiscard]] static std::optional<Texture::Kind> texture_kind() { return Texture::TX_NULL; }

    [[nodiscard]] static std::optional<Pipeline::Kind> pipeline_kind() { return Pipeline::BACK_DROP; }
    [[nodiscard]] static std::optional<DescriptorSet::Kind> descriptor_set_kind() { return Descriptor::BACK_DROP; }
};

class CoverArtSprite : public Sprite {
public:
    CoverArtSprite(const std::shared_ptr<Device> &device, TextureManager &texture_manager,
                   PipelineManager &pipeline_manager, UniformBufferManager &uniform_buffer_manager,
                   const std::shared_ptr<VertexBuffer> &vertex_buffer, std::shared_ptr<DescriptorPool> &descriptor_pool,
                   const size_t image_count) : Sprite(Pipeline::COVER_ART, Model::COVER_ART, pipeline_manager,
                                                      vertex_buffer, descriptor_pool, image_count)
    {
        set_buffer_bindings({0});
        set_binding_buffer_size({sizeof(Camera::Data)});
        set_binding_buffer_kind({
            UniformBufferManager::CAMERA
        });

        set_image_bindings({1});
        set_binding_image_kind({Texture::LOCAL});

        set_descriptor_set_kind(Descriptor::COVER_ART);
        set_pipeline_kind(Pipeline::COVER_ART);
        set_model_kind(Model::COVER_ART);

        set_descriptor_sets(device, texture_manager, uniform_buffer_manager, image_count);
    }

    [[nodiscard]] static std::optional<Model::Kind> model_kind() { return Model::COVER_ART; }
    [[nodiscard]] static std::optional<Pipeline::Kind> pipeline_kind() { return Pipeline::COVER_ART; }
    [[nodiscard]] static std::optional<DescriptorSet::Kind> descriptor_set_kind() { return Descriptor::COVER_ART; }
};


#endif //SPRITE_H
