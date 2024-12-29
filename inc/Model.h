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
class LoadObjModel final : public LoadModel {
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

class Model {
public:
    enum Kind {
        VIKING_ROOM = 0,
        BAR,
        BACK_DROP,
        COVER_ART
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

    static const char* get_obj_path(const Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return MODEL_PATH.c_str();
            case BAR:
                return "/Users/sebastian/CLionProjects/soundscape/models/bar.obj";
            case BACK_DROP:
                return "/Users/sebastian/CLionProjects/soundscape/models/backdrop.obj";
            case COVER_ART:
                return "/Users/sebastian/CLionProjects/soundscape/models/cover_art.obj";
            default:
                throw std::runtime_error("Unknown model kind");
        }
    }

    static LoadModel* load_model(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return new LoadObjModel<StandardVertex>(get_obj_path(kind));
            case BAR:
                return new LoadObjModel<BarVertex>(get_obj_path(kind));
            case BACK_DROP:
                return new LoadObjModel<BackDropVertex>(get_obj_path(kind));
            case COVER_ART:
                return new LoadObjModel<CoverArtVertex>(get_obj_path(kind));
        }
        throw std::runtime_error("Unknown model kind");
    }
};

#endif //MODEL_H
