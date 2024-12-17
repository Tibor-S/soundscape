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
    template<typename T>
    T* get_vertex_mut(const size_t index) {
        return reinterpret_cast<T*>(m_vertices.data()) + index * sizeof(T);
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
    LoadBarModel(double radius, double cylinder_height, size_t lat_res, size_t lon_res) : LoadModel(sizeof(BarVertex)) {
        std::unordered_map<BarVertex, size_t> unique_vertices;
        std::vector<BarVertex> vertices{};
        std::map<size_t, glm::vec3> normals;

        auto z_offset = cylinder_height / 2;

        auto float_lat_res = static_cast<double>(lat_res);
        auto float_lon_res = static_cast<double>(lon_res);
        for (size_t i = 0; i < lat_res + 1; i++) {
            auto float_i_top = static_cast<double>(i);
            auto float_i_bot = static_cast<double>(i + 1);

            auto u_top_deg = M_PI / 2 + M_PI * float_i_top / float_lat_res;
            auto u_bot_deg = M_PI / 2 + M_PI * float_i_bot / float_lat_res;

            auto z_top = radius * glm::sin(u_top_deg);
            auto z_bot = radius * glm::sin(u_bot_deg);

            auto horizontal_radius_top = radius * glm::cos(u_top_deg);
            auto horizontal_radius_bot = radius * glm::cos(u_bot_deg);

            for (size_t j = 0; j < lon_res; j++) {
                auto float_j_left = static_cast<double>(j);
                auto float_j_right = static_cast<double>(j + 1);

                auto v_left = 2 * M_PI * float_j_left / float_lon_res;
                auto v_right = 2 * M_PI * float_j_right / float_lon_res;

                auto color1 = glm::vec3(1.0, 1.0, 0.0);//glm::vec3((glm::sin(u_top_deg) + 1) / 2, (glm::cos(v_left) + 1) / 2, 0.0f);
                auto color2 = glm::vec3(1.0, 1.0, 0.0);//glm::vec3((glm::sin(u_top_deg) + 1) / 2, (glm::cos(v_right) + 1) / 2, 0.0f);
                auto color3 = glm::vec3(1.0, 1.0, 0.0);//glm::vec3((glm::sin(u_bot_deg) + 1) / 2, (glm::cos(v_left) + 1) / 2, 0.0f);
                auto color4 = glm::vec3(1.0, 1.0, 0.0);//glm::vec3((glm::sin(u_bot_deg) + 1) / 2, (glm::cos(v_right) + 1) / 2, 0.0f);

                auto x_top_left = horizontal_radius_top * glm::cos(v_left);
                auto y_top_left = horizontal_radius_top * glm::sin(v_left);
                auto z_top_left = z_top > 0 ? z_offset + z_top : - z_offset + z_top;
                auto pos_top_left = glm::vec3(x_top_left, y_top_left, z_top_left);

                auto x_top_right = horizontal_radius_top * glm::cos(v_right);
                auto y_top_right = horizontal_radius_top * glm::sin(v_right);
                auto z_top_right = z_top > 0 ? z_offset + z_top : - z_offset + z_top;
                auto pos_top_right = glm::vec3(x_top_right, y_top_right, z_top_right);

                auto x_bot_left = horizontal_radius_bot * glm::cos(v_left);
                auto y_bot_left = horizontal_radius_bot * glm::sin(v_left);
                auto z_bot_left = z_bot > 0 ? z_offset + z_bot : - z_offset + z_bot;
                auto pos_bot_left = glm::vec3(x_bot_left, y_bot_left, z_bot_left);

                auto x_bot_right = horizontal_radius_bot * glm::cos(v_right);
                auto y_bot_right = horizontal_radius_bot * glm::sin(v_right);
                auto z_bot_right = z_bot > 0 ? z_offset + z_bot : - z_offset + z_bot;
                auto pos_bot_right = glm::vec3(x_bot_right, y_bot_right, z_bot_right);

                glm::int32 b_index_top =  z_top > 0 ? 0 : 1;
                glm::int32 b_index_bot =  z_bot > 0 ? 0 : 1;

                auto vert_top_left = BarVertex {.pos = pos_top_left, .color = color1, .b_index = b_index_top};
                auto vert_top_right = BarVertex {.pos = pos_top_right, .color = color2, .b_index = b_index_top};
                auto vert_bot_left = BarVertex {.pos = pos_bot_left, .color = color3, .b_index = b_index_bot};
                auto vert_bot_right = BarVertex {.pos = pos_bot_right, .color = color4, .b_index = b_index_bot};

                if (!unique_vertices.contains(vert_top_left)) {
                    unique_vertices[vert_top_left] = static_cast<uint32_t>(unique_vertices.size());
                    vertices.push_back(vert_top_left);
                }
                if (!unique_vertices.contains(vert_top_right)) {
                    unique_vertices[vert_top_right] = static_cast<uint32_t>(unique_vertices.size());
                    vertices.push_back(vert_top_right);
                }
                if (!unique_vertices.contains(vert_bot_left)) {
                    unique_vertices[vert_bot_left] = static_cast<uint32_t>(unique_vertices.size());
                    vertices.push_back(vert_bot_left);
                }
                if (!unique_vertices.contains(vert_bot_right)) {
                    unique_vertices[vert_bot_right] = static_cast<uint32_t>(unique_vertices.size());
                    vertices.push_back(vert_bot_right);
                }

                if (!normals.contains(unique_vertices[vert_top_left])) {
                    normals[unique_vertices[vert_top_left]] = glm::vec3(0.0f, 0.0f, 0.0f);
                }
                if (!normals.contains(unique_vertices[vert_top_right])) {
                    normals[unique_vertices[vert_top_right]] = glm::vec3(0.0f, 0.0f, 0.0f);
                }
                if (!normals.contains(unique_vertices[vert_bot_left])) {
                    normals[unique_vertices[vert_bot_left]] = glm::vec3(0.0f, 0.0f, 0.0f);
                }
                if (!normals.contains(unique_vertices[vert_bot_right])) {
                    normals[unique_vertices[vert_bot_right]] = glm::vec3(0.0f, 0.0f, 0.0f);
                }

                append_index(unique_vertices[vert_top_left]);
                append_index(unique_vertices[vert_bot_left]);
                append_index(unique_vertices[vert_bot_right]);

                append_index(unique_vertices[vert_top_left]);
                append_index(unique_vertices[vert_bot_right]);
                append_index(unique_vertices[vert_top_right]);

                auto face_left_normal = glm::cross(vert_bot_left.pos - vert_top_left.pos,
                                                   vert_bot_right.pos - vert_top_left.pos);
                auto face_right_normal = glm::cross(vert_bot_right.pos - vert_top_left.pos,
                                                   vert_top_right.pos - vert_top_left.pos);

                normals[unique_vertices[vert_top_left]] += face_left_normal + face_right_normal;
                normals[unique_vertices[vert_top_right]] += face_right_normal;
                normals[unique_vertices[vert_bot_left]] += face_left_normal;
                normals[unique_vertices[vert_bot_right]] += face_left_normal + face_right_normal;
            };
        }

        for (size_t i = 0; i < vertices.size(); i++) {
            vertices[i].normal = normalize(normals[i]);
            append_vertex(vertices[i]);
        }

        // for (size_t i = 0; i < lon_res; i++) {
        //     auto deg1 = 360 * static_cast<float>(i) / static_cast<float>(lon_res);
        //     auto deg2 = 360 * static_cast<float>(i + 1) / static_cast<float>(lon_res);
        //     auto ang1 = glm::radians(deg1);
        //     auto ang2 = glm::radians(deg2);
        //
        //     auto v1 = BarVertex {
        //         .pos = glm::vec3(glm::cos(ang1), sin(ang1), z_offset),
        //         .color = glm::vec3(1.0f, 1.0f, 1.0f),
        //         .b_index = 0
        //     };
        //     auto v2 = BarVertex {
        //         .pos = glm::vec3(glm::cos(ang2), sin(ang2), z_offset),
        //         .color = glm::vec3(1.0f, 1.0f, 1.0f),
        //         .b_index = 0
        //     };
        //     auto v3 = BarVertex {
        //         .pos = glm::vec3(glm::cos(ang1), sin(ang1), -z_offset),
        //         .color = glm::vec3(1.0f, 1.0f, 1.0f),
        //         .b_index = 1
        //     };
        //     auto v4 = BarVertex {
        //         .pos = glm::vec3(glm::cos(ang2), sin(ang2), -z_offset),
        //         .color = glm::vec3(1.0f, 1.0f, 1.0f),
        //         .b_index = 1
        //     };
        //
        //     if (!unique_vertices.contains(v1)) {
        //         unique_vertices[v1] = static_cast<uint32_t>(unique_vertices.size());
        //         append_vertex(v1);
        //     }
        //     if (!unique_vertices.contains(v2)) {
        //         unique_vertices[v2] = static_cast<uint32_t>(unique_vertices.size());
        //         append_vertex(v2);
        //     }
        //     if (!unique_vertices.contains(v3)) {
        //         unique_vertices[v3] = static_cast<uint32_t>(unique_vertices.size());
        //         append_vertex(v3);
        //     }
        //     if (!unique_vertices.contains(v4)) {
        //         unique_vertices[v4] = static_cast<uint32_t>(unique_vertices.size());
        //         append_vertex(v4);
        //     }
        //
        //     // Top circle
        //     append_index(unique_vertices[top_vertex]);
        //     append_index(unique_vertices[v1]);
        //     append_index(unique_vertices[v2]);
        //
        //     // Bot circle
        //     append_index(unique_vertices[bot_vertex]);
        //     append_index(unique_vertices[v4]);
        //     append_index(unique_vertices[v3]);
        //
        //     // Bridge
        //     append_index(unique_vertices[v2]);
        //     append_index(unique_vertices[v1]);
        //     append_index(unique_vertices[v3]);
        //
        //     append_index(unique_vertices[v3]);
        //     append_index(unique_vertices[v4]);
        //     append_index(unique_vertices[v2]);
        // }
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
                return new LoadBarModel(1, 1, 24, 24);
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
