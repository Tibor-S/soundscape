//
// Created by Sebastian Sandstig on 2024-12-06.
//

#ifndef MODEL_H
#define MODEL_H
#include <Globals.h>
#include <Texture.h>
#include <tiny_obj_loader.h>
#include <Vertex.h>
#include <iostream>
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
    // void set_vertices(std::pmr::vector<StandardVertex>& vertices) { return &m_vertices; }
    // void set_indices(std::pmr::vector<StandardVertex>& vertices) { return &m_indices; }
private:
    size_t m_vertex_size;
    std::vector<char> m_vertices;
    std::vector<uint32_t> m_indices;
};

template <typename T>
class LoadObjModel : public LoadModel {
public:
    explicit LoadObjModel(const char* obj_path) : LoadModel(sizeof(T)) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, obj_path)) {
            throw std::runtime_error(warn + err);
        }

        auto object = Object3D();
        // std::unordered_map<SourceVertex, uint32_t> unique_vertices{};
        std::unordered_map<std::string, size_t> vertex_group_names{};
        std::vector<glm::i8vec2> vertex_groups {};
        auto normals = std::vector<glm::vec3>();
        // auto vertices = std::vector<SourceVertex>();
        for (const auto& shape : shapes) {
            if (!vertex_group_names.contains(shape.name)) {
                vertex_group_names[shape.name] = vertex_group_names.size();
            }

            for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
                std::array<ObjVertex::Unique, 3> face{};
                tinyobj::index_t data_index{};
                glm::vec3 face_normal = {
                    attrib.normals[3 * shape.mesh.indices[i].normal_index + 0],
                    attrib.normals[3 * shape.mesh.indices[i].normal_index + 1],
                    attrib.normals[3 * shape.mesh.indices[i].normal_index + 2],
                };

                for (size_t face_index = 0; face_index < 3; face_index++) {
                    data_index = shape.mesh.indices[i + face_index];

                    face[face_index].pos = {
                        attrib.vertices[3 * data_index.vertex_index + 0],
                        attrib.vertices[3 * data_index.vertex_index + 1],
                        attrib.vertices[3 * data_index.vertex_index + 2]
                    };

                    face[face_index].color = {1.0f, 1.0f, 1.0f};

                    face[face_index].tex_coord = {
                        attrib.texcoords[2 * data_index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * data_index.texcoord_index + 1]
                    };

                }

                object.add_face(face, face_normal, vertex_group_names[shape.name]);

                // if (!unique_vertices.contains(source_vertex)) {
                //     unique_vertices[source_vertex] = static_cast<uint32_t>(unique_vertices.size());
                //     // append_vertex(source_vertex);
                //     vertices.push_back(source_vertex);
                //     normals.emplace_back(0.0f, 0.0f, 0.0f);
                //     vertex_groups.emplace_back(vertex_group_names[shape.name], -1);
                //     // auto serialized_vertex = reinterpret_cast<char*>(&vertex);
                //     // for (int i = 0; i < sizeof(vertex); i++) {
                //     //     vertices->push_back(serialized_vertex[i]);
                //     // }
                // } else if (vertex_groups[unique_vertices[source_vertex]].x !=  vertex_group_names[shape.name]) {
                //     // if (vertex_groups[unique_vertices[source_vertex]].y == -1) {
                //     //
                //     // }
                // }

                // normals[unique_vertices[source_vertex]].x += attrib.normals[3 * index.normal_index + 0];
                // normals[unique_vertices[source_vertex]].y += attrib.normals[3 * index.normal_index + 1];
                // normals[unique_vertices[source_vertex]].z += attrib.normals[3 * index.normal_index + 2];
                // append_index(unique_vertices[source_vertex]);
                // indices->push_back(unique_vertices[vertex]);
            }
        }

        auto vertices = object.get_vertices<T>();
        for (size_t i = 0; i < vertices.size(); i++) {
            // SourceVertex source_vertex = vertices[i];
            // source_vertex.normal = glm::normalize(normals[i]);
            // std::cout << "t.x: " << source_vertex.tex_coord.x << ", t.y: " << source_vertex.tex_coord.y << std::endl;
            // std::cout << "\tonormal:\t" << normals[i].x << ", " << normals[i].y << ", " << normals[i].z << std::endl;
            // std::cout << "\tnormal:\t" << source_vertex.normal.x << ", " << source_vertex.normal.y << ", " << source_vertex.normal.z << std::endl;
            // T vertex(source_vertex);

            append_vertex(vertices[i]);
        }
        for (auto index : object.get_indices()) {
            append_index(index);
        }
    }
};


class LoadBarModel : public LoadModel {
public:
    LoadBarModel(double radius, double cylinder_height, size_t lat_res, size_t lon_res) : LoadModel(sizeof(BarVertex)) {
        // std::unordered_map<BarVertex, size_t> unique_vertices;
        // std::vector<BarVertex> vertices{};
        // std::map<size_t, glm::vec3> normals;
        //
        // auto z_offset = cylinder_height / 2;
        //
        // auto float_lat_res = static_cast<double>(lat_res);
        // auto float_lon_res = static_cast<double>(lon_res);
        // for (size_t i = 0; i < lat_res + 1; i++) {
        //     auto float_i_top = static_cast<double>(i);
        //     auto float_i_bot = static_cast<double>(i + 1);
        //
        //     auto u_top_deg = M_PI / 2 + M_PI * float_i_top / float_lat_res;
        //     auto u_bot_deg = M_PI / 2 + M_PI * float_i_bot / float_lat_res;
        //
        //     auto z_top = radius * glm::sin(u_top_deg);
        //     auto z_bot = radius * glm::sin(u_bot_deg);
        //
        //     auto horizontal_radius_top = radius * glm::cos(u_top_deg);
        //     auto horizontal_radius_bot = radius * glm::cos(u_bot_deg);
        //
        //     for (size_t j = 0; j < lon_res; j++) {
        //         auto float_j_left = static_cast<double>(j);
        //         auto float_j_right = static_cast<double>(j + 1);
        //
        //         auto v_left = 2 * M_PI * float_j_left / float_lon_res;
        //         auto v_right = 2 * M_PI * float_j_right / float_lon_res;
        //
        //         auto color1 = glm::vec3(1.0, 1.0, 0.0);
        //         auto color2 = glm::vec3(1.0, 1.0, 0.0);
        //         auto color3 = glm::vec3(1.0, 1.0, 0.0);
        //         auto color4 = glm::vec3(1.0, 1.0, 0.0);
        //
        //         auto x_top_left = horizontal_radius_top * glm::cos(v_left);
        //         auto y_top_left = horizontal_radius_top * glm::sin(v_left);
        //         auto z_top_left = z_top > 0 ? z_offset + z_top : - z_offset + z_top;
        //         auto pos_top_left = glm::vec3(x_top_left, y_top_left, z_top_left);
        //
        //         auto x_top_right = horizontal_radius_top * glm::cos(v_right);
        //         auto y_top_right = horizontal_radius_top * glm::sin(v_right);
        //         auto z_top_right = z_top > 0 ? z_offset + z_top : - z_offset + z_top;
        //         auto pos_top_right = glm::vec3(x_top_right, y_top_right, z_top_right);
        //
        //         auto x_bot_left = horizontal_radius_bot * glm::cos(v_left);
        //         auto y_bot_left = horizontal_radius_bot * glm::sin(v_left);
        //         auto z_bot_left = z_bot > 0 ? z_offset + z_bot : - z_offset + z_bot;
        //         auto pos_bot_left = glm::vec3(x_bot_left, y_bot_left, z_bot_left);
        //
        //         auto x_bot_right = horizontal_radius_bot * glm::cos(v_right);
        //         auto y_bot_right = horizontal_radius_bot * glm::sin(v_right);
        //         auto z_bot_right = z_bot > 0 ? z_offset + z_bot : - z_offset + z_bot;
        //         auto pos_bot_right = glm::vec3(x_bot_right, y_bot_right, z_bot_right);
        //
        //         glm::int32 b_index_top =  z_top > 0 ? 0 : 1;
        //         glm::int32 b_index_bot =  z_bot > 0 ? 0 : 1;
        //
        //         BarVertex vert_top_left = {.pos = pos_top_left, .color = color1, .v_groups = b_index_top};
        //         BarVertex vert_top_right = {.pos = pos_top_right, .color = color2, .v_groups = b_index_top};
        //         BarVertex vert_bot_left = {.pos = pos_bot_left, .color = color3, .v_groups = b_index_bot};
        //         BarVertex vert_bot_right = {.pos = pos_bot_right, .color = color4, .v_groups = b_index_bot};
        //
        //         if (!unique_vertices.contains(vert_top_left)) {
        //             unique_vertices[vert_top_left] = static_cast<uint32_t>(unique_vertices.size());
        //             vertices.push_back(vert_top_left);
        //         }
        //         if (!unique_vertices.contains(vert_top_right)) {
        //             unique_vertices[vert_top_right] = static_cast<uint32_t>(unique_vertices.size());
        //             vertices.push_back(vert_top_right);
        //         }
        //         if (!unique_vertices.contains(vert_bot_left)) {
        //             unique_vertices[vert_bot_left] = static_cast<uint32_t>(unique_vertices.size());
        //             vertices.push_back(vert_bot_left);
        //         }
        //         if (!unique_vertices.contains(vert_bot_right)) {
        //             unique_vertices[vert_bot_right] = static_cast<uint32_t>(unique_vertices.size());
        //             vertices.push_back(vert_bot_right);
        //         }
        //
        //         if (!normals.contains(unique_vertices[vert_top_left])) {
        //             normals[unique_vertices[vert_top_left]] = glm::vec3(0.0f, 0.0f, 0.0f);
        //         }
        //         if (!normals.contains(unique_vertices[vert_top_right])) {
        //             normals[unique_vertices[vert_top_right]] = glm::vec3(0.0f, 0.0f, 0.0f);
        //         }
        //         if (!normals.contains(unique_vertices[vert_bot_left])) {
        //             normals[unique_vertices[vert_bot_left]] = glm::vec3(0.0f, 0.0f, 0.0f);
        //         }
        //         if (!normals.contains(unique_vertices[vert_bot_right])) {
        //             normals[unique_vertices[vert_bot_right]] = glm::vec3(0.0f, 0.0f, 0.0f);
        //         }
        //
        //         append_index(unique_vertices[vert_top_left]);
        //         append_index(unique_vertices[vert_bot_left]);
        //         append_index(unique_vertices[vert_bot_right]);
        //
        //         append_index(unique_vertices[vert_top_left]);
        //         append_index(unique_vertices[vert_bot_right]);
        //         append_index(unique_vertices[vert_top_right]);
        //
        //         auto face_left_normal = glm::cross(vert_bot_left.pos - vert_top_left.pos,
        //                                            vert_bot_right.pos - vert_top_left.pos);
        //         auto face_right_normal = glm::cross(vert_bot_right.pos - vert_top_left.pos,
        //                                            vert_top_right.pos - vert_top_left.pos);
        //
        //         normals[unique_vertices[vert_top_left]] += face_left_normal + face_right_normal;
        //         normals[unique_vertices[vert_top_right]] += face_right_normal;
        //         normals[unique_vertices[vert_bot_left]] += face_left_normal;
        //         normals[unique_vertices[vert_bot_right]] += face_left_normal + face_right_normal;
        //     };
        // }
        //
        // for (size_t i = 0; i < vertices.size(); i++) {
        //     vertices[i].normal = normalize(normals[i]);
        //     append_vertex(vertices[i]);
        // }
    }
};

class Model {
public:
    enum Kind {
        VIKING_ROOM = 0,
        BAR,
        BACK_DROP
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
            case BACK_DROP:
                return "/Users/sebastian/CLionProjects/soundscape/models/backdrop.obj";
            case BAR:
            default:
                return "/Users/sebastian/CLionProjects/soundscape/models/bar.obj";
        }
        throw std::runtime_error("Unknown model kind");
    }

    static LoadModel* load_model(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return new LoadObjModel<StandardVertex>(get_obj_path(kind));
            case BAR:
                return new LoadObjModel<BarVertex>(get_obj_path(kind));//new LoadBarModel(1, 1, 24, 24);
                return new LoadBarModel(1, 1, 24, 24);
            case BACK_DROP:
                return new LoadObjModel<BackDropVertex>(get_obj_path(kind));
        }
        throw std::runtime_error("Unknown model kind");
    }
};

#endif //MODEL_H
