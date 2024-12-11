//
// Created by Sebastian Sandstig on 2024-12-04.
//

#ifndef SPRITE_H
#define SPRITE_H
#include <Descriptor.h>
#include <DescriptorManager.h>
#include <Model.h>
#include <Pipeline.h>
#include <Texture.h>
#include <TextureImage.h>
#include <UniformBuffer.h>
#include <VertexBuffer.h>

namespace Sprite {

struct Spec {
    VertexBuffer::VertexBuffer* vertex_buffer;
    size_t model_index;
    TextureImage::TextureImage* texture_image;
    Descriptor::DescriptorManager* descriptor_manager;
    Pipeline::Pipeline* pipeline;
    size_t uniform_buffer_count;
    size_t size;
};

class Sprite : public VertexBuffer::VertexBufferParent,
               public TextureImage::TextureImageParent,
               public Descriptor::DescriptorManagerParent,
               public Pipeline::PipelineParent {
public:
    Sprite(Spec &spec) : VertexBufferParent(spec.vertex_buffer), TextureImageParent(spec.texture_image),
                         DescriptorManagerParent(spec.descriptor_manager), PipelineParent(spec.pipeline)
    {
        m_first_vertex = get_vertex_buffer()->get_vertex_offset(spec.model_index);
        m_first_index = get_vertex_buffer()->get_index_offset(spec.model_index);
        m_index_count = get_vertex_buffer()->get_index_count(spec.model_index);

        m_uniform_buffer_count = spec.uniform_buffer_count;
        auto device = get_descriptor_manager()->get_device();
        UniformBuffer::Spec uniform_buffer_spec = {};
        uniform_buffer_spec.device = device;
        uniform_buffer_spec.size = spec.size;
        m_uniform_buffers.resize(spec.uniform_buffer_count);
        for (int i = 0; i < spec.uniform_buffer_count; i++) {
            m_uniform_buffers[i] = new UniformBuffer::UniformBuffer(uniform_buffer_spec);
        }
        m_descriptor_indices = get_descriptor_manager()->get_unique_descriptors(m_uniform_buffer_count);
        auto updater = get_descriptor_manager()->get_updater();

        for (size_t i = 0; i < m_uniform_buffer_count; i++) {
            auto uniform_buffer = m_uniform_buffers[i];
            updater->update_buffer(m_descriptor_indices[i],
                0,
                uniform_buffer->get_handle(),
                0,
                spec.size
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

    UniformBuffer::UniformBuffer* get_uniform_buffer(size_t i) { return m_uniform_buffers[i]; }
    size_t get_descriptor(size_t i) { return m_descriptor_indices[i]; }

    void draw(VkCommandBuffer command_buffer, size_t relative_frame) {
        VkBuffer vertexBuffers[] = {get_vertex_buffer()->get_vertex_handle()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, get_vertex_buffer()->get_index_handle(), 0, VK_INDEX_TYPE_UINT32);

        auto descriptor_index = get_descriptor(relative_frame);//m_descriptor_indices[currentFrame];
        auto descriptor = get_descriptor_manager()->descriptor_handle(descriptor_index);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                get_pipeline()->get_pipeline_layout()->get_handle(), 0, 1, &descriptor, 0, nullptr);
        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(m_index_count), 1, m_first_index, m_first_vertex, 0);

    }
private:
    size_t m_uniform_buffer_count;
    std::vector<UniformBuffer::UniformBuffer*> m_uniform_buffers;
    std::vector<size_t> m_descriptor_indices;

    size_t m_first_vertex;
    size_t m_first_index;
    size_t m_index_count;
};
}
class Sprite2 {
public:
    enum Kind {
        VIKING_ROOM
    };

    explicit Sprite2(std::shared_ptr<VertexBuffer2> &vertex_buffer, std::shared_ptr<TextureImage2> &texture,
                     std::shared_ptr<Pipeline2> &pipeline,
                     std::shared_ptr<DescriptorPool> &descriptor_pool,
                     Kind kind,
                     size_t image_count) : m_vertex_buffer(vertex_buffer),
                                           m_texture(texture),
                                           m_pipeline(pipeline),
                                           m_kind(kind)
    {
        m_descriptor_sets = DescriptorSet::create_descriptor_sets(descriptor_pool, image_count);
        m_vertex_buffer_position = m_vertex_buffer->get_model_position(get_kind_model(m_kind));
    }
    ~Sprite2() = default;

    [[nodiscard]] DescriptorSetUpdater* get_descriptor_set_updater(const size_t index) const {
        return new DescriptorSetUpdater(m_descriptor_sets[index]);
    }
    [[nodiscard]] std::shared_ptr<DescriptorSet> get_descriptor_set(const size_t index) const {
        return m_descriptor_sets[index];
    }
    [[nodiscard]] static Model::Kind get_kind_model(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return Model::VIKING_ROOM;
        }
        throw std::runtime_error("Unknown kind");
    }
    [[nodiscard]] Model::Kind get_kind_model() const { return get_kind_model(m_kind); }
    [[nodiscard]] static Texture::Kind get_kind_texture(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return Texture::VIKING_ROOM;
        }
        throw std::runtime_error("Unknown kind");
    }
    [[nodiscard]] Texture::Kind get_kind_texture() const { return get_kind_texture(m_kind); }
    [[nodiscard]] static Pipeline2::Kind get_kind_pipeline(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return Pipeline2::STANDARD;
        }
        throw std::runtime_error("Unknown kind");
    }
    [[nodiscard]] Pipeline2::Kind get_kind_pipeline() const { return get_kind_pipeline(m_kind); }
    [[nodiscard]] static Descriptor2::Kind get_kind_descriptor(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return Descriptor2::CAMERA_MODEL_SAMPLER;
        }
        throw std::runtime_error("Unknown kind");
    }
    [[nodiscard]] Descriptor2::Kind get_kind_descriptor() const { return get_kind_descriptor(m_kind); }
    [[nodiscard]] std::shared_ptr<VertexBuffer2> get_vertex_buffer() const { return m_vertex_buffer; }
    [[nodiscard]] VertexBuffer2::ModelPosition get_vertex_position() const { return m_vertex_buffer_position; }
private:
    std::shared_ptr<VertexBuffer2> m_vertex_buffer;
    std::shared_ptr<TextureImage2> m_texture;
    std::shared_ptr<Pipeline2> m_pipeline;
    Kind m_kind;

    std::vector<std::shared_ptr<DescriptorSet>> m_descriptor_sets;
    VertexBuffer2::ModelPosition m_vertex_buffer_position {};
};


class Sprite3 {
public:
    Sprite3(Pipeline2::Kind pipeline_kind, Model::Kind model_kind, PipelineManager &pipeline_manager,
            std::shared_ptr<VertexBuffer2> &vertex_buffer, std::shared_ptr<DescriptorPool> &descriptor_pool,
            size_t image_count) : m_vertex_buffer(vertex_buffer)
    {
        m_pipeline = pipeline_manager.acquire_pipeline(pipeline_kind);

        m_descriptor_sets = DescriptorSet::create_descriptor_sets(descriptor_pool, image_count);
        m_vertex_buffer_position = m_vertex_buffer->get_model_position(model_kind);
    }

    void set_descriptor_sets(std::shared_ptr<Device::Device>& device, TextureManager &texture_manager,
            UniformBufferManager &uniform_buffer_manager,size_t image_count) {
        for (size_t i = 0; i < image_count; i++) {
            auto descriptor_set = get_descriptor_set(i);
            auto updater = new DescriptorSetUpdater(descriptor_set);
            std::vector<std::shared_ptr<UniformBuffer::UniformBuffer>> uniform_buffers;
            std::vector<std::shared_ptr<TextureImage2>> textures;
            // m_buffers.emplace_back();
            // m_textures.emplace_back();

            for (const auto binding : m_buffer_bindings) {
                auto kind = binding_buffer_kind(binding);
                if (kind == UniformBufferManager::LOCAL) {
                    auto size = binding_buffer_size(binding);
                    if (!size) {
                        throw std::runtime_error("Buffer size is not set for binding");
                    }
                    UniformBuffer::Spec spec = {
                        .device = device.get(),
                        .size = size,
                    };
                    auto buffer = std::make_shared<UniformBuffer::UniformBuffer>(spec);
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
                auto image = texture_manager.acquire_texture(kind);
                textures.push_back(image);
                updater->update_image(binding, *image);
            }

            updater->finalize();

            delete updater;

            m_buffers.push_back(uniform_buffers);
            m_textures.push_back(textures);
        }
    }

    // [[nodiscard]] std::vector<size_t> buffer_bindings() {return m_buffer_bindings; }
    // [[nodiscard]] std::vector<size_t> image_bindings() {return m_image_bindings; }
    [[nodiscard]] UniformBufferManager::Kind binding_buffer_kind(size_t binding) const {
        return m_binding_buffer_kind[get_buffer_binding_index(binding)];
    }
    [[nodiscard]] size_t binding_buffer_size(size_t binding) const {
        return m_binding_buffer_size[get_buffer_binding_index(binding)];
    }
    [[nodiscard]] Texture::Kind binding_image_kind(size_t binding) const {
        return m_binding_image_kind[get_image_binding_index(binding)];
    }
    [[nodiscard]] std::optional<Model::Kind> model_kind() { return m_model_kind; }
    [[nodiscard]] std::optional<Texture::Kind> texture_kind() { return m_texture_kind; }
    [[nodiscard]] std::optional<Pipeline2::Kind> pipeline_kind() { return m_pipeline_kind; }
    [[nodiscard]] std::optional<DescriptorSet::Kind> descriptor_set_kind() { return m_descriptor_set_kind; }
    [[nodiscard]] std::shared_ptr<DescriptorSet> get_descriptor_set(const size_t index) const {
        return m_descriptor_sets[index];
    }
    [[nodiscard]] std::shared_ptr<VertexBuffer2> get_vertex_buffer() const { return m_vertex_buffer; }
    [[nodiscard]] VertexBuffer2::ModelPosition get_vertex_position() const { return m_vertex_buffer_position; }
    [[nodiscard]] std::shared_ptr<UniformBuffer::UniformBuffer> get_buffer(size_t image_index, size_t binding) const {
        return m_buffers[image_index][get_buffer_binding_index(binding)];
    }
    [[nodiscard]] std::shared_ptr<TextureImage2> get_image(size_t image_index, size_t binding) const {
        return m_textures[image_index][get_image_binding_index(binding)];
    }

    void set_buffer(size_t image_index, size_t binding, void* data, size_t size) const {
        auto buffer = get_buffer(image_index, binding);
        buffer->update(data, size);
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
    void set_model_kind(Model::Kind kind) { m_model_kind = kind; }
    void set_texture_kind(Texture::Kind kind) { m_texture_kind = kind; }
    void set_pipeline_kind(Pipeline2::Kind kind) { m_pipeline_kind = kind; }
    void set_descriptor_set_kind(DescriptorSet::Kind kind) { m_descriptor_set_kind = kind; }
private:
    std::vector<std::shared_ptr<DescriptorSet>> m_descriptor_sets;
    std::vector<std::vector<std::shared_ptr<TextureImage2>>> m_textures;
    std::vector<std::vector<std::shared_ptr<UniformBuffer::UniformBuffer>>> m_buffers;
    std::shared_ptr<Pipeline2> m_pipeline;
    std::shared_ptr<VertexBuffer2> m_vertex_buffer;
    VertexBuffer2::ModelPosition m_vertex_buffer_position {};

    std::vector<size_t> m_buffer_bindings;
    std::vector<size_t> m_image_bindings;
    std::vector<UniformBufferManager::Kind> m_binding_buffer_kind;
    std::vector<size_t> m_binding_buffer_size;
    std::vector<Texture::Kind> m_binding_image_kind;

    Model::Kind m_model_kind {};
    Texture::Kind m_texture_kind {};
    Pipeline2::Kind m_pipeline_kind {};
    DescriptorSet::Kind m_descriptor_set_kind {};


    [[nodiscard]] size_t get_buffer_binding_index(const size_t binding) const {
        auto bindings = m_buffer_bindings;
        size_t buffer_index = -1;
        if (const auto index = std::ranges::find(bindings, binding); index != bindings.end()) {
            buffer_index = index - bindings.begin();
        } else {
            throw std::runtime_error("No buffer present on binding");
        }
        return buffer_index;
    }
    [[nodiscard]] size_t get_image_binding_index(const size_t binding) const {
        auto bindings = m_image_bindings;
        size_t image_index = -1;
        if (const auto index = std::ranges::find(bindings, binding); index != bindings.end()) {
            image_index = index - bindings.begin();
        } else {
            throw std::runtime_error("No image present on binding");
        }
        return image_index;
    }
};

struct SpriteModel {
    glm::mat4 model_matrix;
};

class CameraModelSamplerSprite : public Sprite3 {
public:
    CameraModelSamplerSprite(Model::Kind model_kind, PipelineManager &pipeline_manager,
                             std::shared_ptr<VertexBuffer2> &vertex_buffer,
                             std::shared_ptr<DescriptorPool> &descriptor_pool,
                             size_t image_count) : Sprite3(Pipeline2::STANDARD, model_kind, pipeline_manager, vertex_buffer, descriptor_pool,
                                                           image_count)
    {
        set_buffer_bindings({0, 1});
        set_image_bindings({2});
        set_binding_buffer_kind({
            UniformBufferManager::CAMERA, UniformBufferManager::LOCAL
        });
        set_binding_buffer_size({sizeof(Camera::Data), sizeof(SpriteModel)});
        set_binding_image_kind({Texture::TX_NULL});
        set_pipeline_kind(Pipeline2::STANDARD);
        set_descriptor_set_kind(Descriptor2::CAMERA_MODEL_SAMPLER);
    }

    [[nodiscard]] static std::optional<Pipeline2::Kind> pipeline_kind() { return Pipeline2::STANDARD; }
    [[nodiscard]] static std::optional<Descriptor2::Kind> descriptor_set_kind() {
        return Descriptor2::CAMERA_MODEL_SAMPLER;
    }

    void set_sprite_model(SpriteModel& sprite_model, size_t image_index) const {
        set_buffer(image_index, 1, &sprite_model, sizeof(SpriteModel));
    }
};


class VikingRoom : public CameraModelSamplerSprite {
public:
    VikingRoom(std::shared_ptr<Device::Device> &device, TextureManager &texture_manager,
               PipelineManager &pipeline_manager,
               UniformBufferManager &uniform_buffer_manager,
               std::shared_ptr<VertexBuffer2> &vertex_buffer,
               std::shared_ptr<DescriptorPool> &descriptor_pool,
               size_t image_count) : CameraModelSamplerSprite(Model::VIKING_ROOM, pipeline_manager, vertex_buffer, descriptor_pool,
                                                              image_count)
    {
        set_binding_image_kind({Texture::VIKING_ROOM});
        set_model_kind(Model::VIKING_ROOM);
        set_texture_kind(Texture::VIKING_ROOM);
        set_descriptor_sets(device, texture_manager, uniform_buffer_manager, image_count);
    }

    [[nodiscard]] static std::optional<Model::Kind> model_kind() { return Model::VIKING_ROOM; }
    [[nodiscard]] static std::optional<Texture::Kind> texture_kind() { return Texture::VIKING_ROOM; }
};


struct BoneBuffer {
    glm::mat4 bone[2];
};


class CameraModelBufferSprite : public Sprite3 {
public:
    CameraModelBufferSprite(Model::Kind model_kind, PipelineManager &pipeline_manager,
                             std::shared_ptr<VertexBuffer2> &vertex_buffer,
                             std::shared_ptr<DescriptorPool> &descriptor_pool,
                             size_t image_count) : Sprite3(Pipeline2::BAR, model_kind, pipeline_manager, vertex_buffer, descriptor_pool,
                                                           image_count)
    {
        set_buffer_bindings({0, 1, 2});
        set_image_bindings({});
        set_binding_buffer_kind({
            UniformBufferManager::CAMERA, UniformBufferManager::LOCAL, UniformBufferManager::LOCAL
        });
        set_binding_buffer_size({sizeof(Camera::Data), sizeof(SpriteModel), sizeof(BoneBuffer)});
        set_binding_image_kind({Texture::TX_NULL});
        set_pipeline_kind(Pipeline2::BAR);
        set_descriptor_set_kind(Descriptor2::CAMERA_MODEL_BUFFER);
    }

    [[nodiscard]] static std::optional<Pipeline2::Kind> pipeline_kind() { return Pipeline2::BAR; }
    [[nodiscard]] static std::optional<Descriptor2::Kind> descriptor_set_kind() {
        return Descriptor2::CAMERA_MODEL_BUFFER;
    }

    void set_sprite_model(SpriteModel& sprite_model, size_t image_index) const {
        set_buffer(image_index, 1, &sprite_model, sizeof(SpriteModel));
    }

    void set_bone_model(BoneBuffer& bone_buffer, size_t image_index) const {
        set_buffer(image_index, 2, &bone_buffer, sizeof(BoneBuffer));
    }
};

class BarSprite : public CameraModelBufferSprite {
public:
    BarSprite(std::shared_ptr<Device::Device> &device, TextureManager &texture_manager,
               PipelineManager &pipeline_manager,
               UniformBufferManager &uniform_buffer_manager,
               std::shared_ptr<VertexBuffer2> &vertex_buffer,
               std::shared_ptr<DescriptorPool> &descriptor_pool,
               size_t image_count) : CameraModelBufferSprite(Model::BAR, pipeline_manager, vertex_buffer, descriptor_pool,
                                                              image_count)
    {
        set_binding_image_kind({Texture::TX_NULL});
        set_model_kind(Model::BAR);
        set_texture_kind(Texture::TX_NULL);
        set_descriptor_sets(device, texture_manager, uniform_buffer_manager, image_count);
    }

    [[nodiscard]] static std::optional<Model::Kind> model_kind() { return Model::BAR; }
    [[nodiscard]] static std::optional<Texture::Kind> texture_kind() { return Texture::TX_NULL; }
};

#endif //SPRITE_H
