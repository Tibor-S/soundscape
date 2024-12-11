//
// Created by Sebastian Sandstig on 2024-12-06.
//

#ifndef MODEL_H
#define MODEL_H
#include <Globals.h>
#include <Texture.h>
#include <tiny_obj_loader.h>
#include <Vertex.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

// Load Model

class LoadModel {
public:
    explicit LoadModel(size_t vertex_size) : m_vertex_size(vertex_size) {}
    virtual  ~LoadModel() = default;

    [[nodiscard]] size_t get_vertex_size() const { return m_vertex_size; }
    [[nodiscard]] std::vector<char>* get_vertices() { return &m_vertices; }
    [[nodiscard]] std::vector<uint32_t>* get_indices() { return &m_indices; }

    template<typename T>
    void append_vertex(T& vertex) {
        auto serialized_vertex = reinterpret_cast<char*>(&vertex);
        for (int i = 0; i < sizeof(vertex); i++) {
            m_vertices.push_back(serialized_vertex[i]);
        }
    }
    void append_index(uint32_t index) {
        m_indices.push_back(index);
    }
    // void set_vertices(std::pmr::vector<Vertex>& vertices) { return &m_vertices; }
    // void set_indices(std::pmr::vector<Vertex>& vertices) { return &m_indices; }
private:
    size_t m_vertex_size;
    std::vector<char> m_vertices;
    std::vector<uint32_t> m_indices;
};

class LoadObjModel : public LoadModel {
public:
    explicit LoadObjModel(const char* obj_path) : LoadModel(sizeof(Vertex)) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, obj_path)) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> unique_vertices{};

        auto vertices = get_vertices();
        auto indices = get_indices();
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (!unique_vertices.contains(vertex)) {
                    unique_vertices[vertex] = static_cast<uint32_t>(unique_vertices.size());

                    auto serialized_vertex = reinterpret_cast<char*>(&vertex);
                    for (int i = 0; i < sizeof(vertex); i++) {
                        vertices->push_back(serialized_vertex[i]);
                    }
                }

                indices->push_back(unique_vertices[vertex]);
            }
        }
    }
    ~LoadObjModel() = default;
};


class LoadBarModel : public LoadModel {
public:
    LoadBarModel(size_t lat_res, size_t lon_res) : LoadModel(sizeof(BarVertex)) {
        std::unordered_map<BarVertex, size_t> unique_vertices;

        auto bh = 0.5f;
        auto top_vertex = BarVertex {
            .pos = glm::vec3(0.0f, 0.0f, bh),
            .color = glm::vec3(1.0f, 1.0f, 1.0f),
            .b_index = 0
        };
        auto bot_vertex = BarVertex {
            .pos = glm::vec3(0.0f, 0.0f, -bh),
            .color = glm::vec3(1.0f, 1.0f, 1.0f),
            .b_index = 1
        };

        unique_vertices[top_vertex] = static_cast<uint32_t>(unique_vertices.size());
        append_vertex(top_vertex);
        unique_vertices[bot_vertex] = static_cast<uint32_t>(unique_vertices.size());
        append_vertex(bot_vertex);

        for (size_t i = 0; i < lon_res; i++) {
            auto deg1 = 360 * static_cast<float>(i) / static_cast<float>(lon_res);
            auto deg2 = 360 * static_cast<float>(i + 1) / static_cast<float>(lon_res);
            auto ang1 = glm::radians(deg1);
            auto ang2 = glm::radians(deg2);

            auto v1 = BarVertex {
                .pos = glm::vec3(glm::cos(ang1), sin(ang1), bh),
                .color = glm::vec3(1.0f, 1.0f, 1.0f),
                .b_index = 0
            };
            auto v2 = BarVertex {
                .pos = glm::vec3(glm::cos(ang2), sin(ang2), bh),
                .color = glm::vec3(1.0f, 1.0f, 1.0f),
                .b_index = 0
            };
            auto v3 = BarVertex {
                .pos = glm::vec3(glm::cos(ang1), sin(ang1), -bh),
                .color = glm::vec3(1.0f, 1.0f, 1.0f),
                .b_index = 1
            };
            auto v4 = BarVertex {
                .pos = glm::vec3(glm::cos(ang2), sin(ang2), -bh),
                .color = glm::vec3(1.0f, 1.0f, 1.0f),
                .b_index = 1
            };

            if (!unique_vertices.contains(v1)) {
                unique_vertices[v1] = static_cast<uint32_t>(unique_vertices.size());
                append_vertex(v1);
            }
            if (!unique_vertices.contains(v2)) {
                unique_vertices[v2] = static_cast<uint32_t>(unique_vertices.size());
                append_vertex(v2);
            }
            if (!unique_vertices.contains(v3)) {
                unique_vertices[v3] = static_cast<uint32_t>(unique_vertices.size());
                append_vertex(v3);
            }
            if (!unique_vertices.contains(v4)) {
                unique_vertices[v4] = static_cast<uint32_t>(unique_vertices.size());
                append_vertex(v4);
            }

            // Top circle
            append_index(unique_vertices[top_vertex]);
            append_index(unique_vertices[v1]);
            append_index(unique_vertices[v2]);

            // Bot circle
            append_index(unique_vertices[bot_vertex]);
            append_index(unique_vertices[v4]);
            append_index(unique_vertices[v3]);

            // Bridge
            append_index(unique_vertices[v2]);
            append_index(unique_vertices[v1]);
            append_index(unique_vertices[v3]);

            append_index(unique_vertices[v3]);
            append_index(unique_vertices[v4]);
            append_index(unique_vertices[v2]);
        }

    }
};

class Model {
public:
    enum Kind {
        VIKING_ROOM = 0,
        BAR,
    };

    explicit Model(const Kind kind) : m_loaded_model(load_model(kind)) {
        m_kind = kind;
    }
    ~Model() {
        delete m_loaded_model;
    }

    [[nodiscard]] Kind get_kind() const { return m_kind; }
    [[nodiscard]] LoadModel* get_loaded_model() const { return m_loaded_model; }
private:
    Kind m_kind;
    LoadModel* m_loaded_model = nullptr;

    static const char* get_obj_path(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return MODEL_PATH.c_str();
            default:
                return "/Users/sebastian/CLionProjects/soundscape/models/bar.obj";
        }
        throw std::runtime_error("Unknown model kind");
    }

    static LoadModel* load_model(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return new LoadObjModel(get_obj_path(kind));
            case BAR:
                return new LoadBarModel(16, 16);
        }
        throw std::runtime_error("Unknown model kind");
    }
};

class VikingRoomModel : public LoadObjModel {
public:
    VikingRoomModel() : LoadObjModel(MODEL_PATH.c_str()) {}
    ~VikingRoomModel() override = default;
};

#endif //MODEL_H
