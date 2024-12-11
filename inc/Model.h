//
// Created by Sebastian Sandstig on 2024-12-06.
//

#ifndef MODEL_H
#define MODEL_H
#include <Globals.h>
#include <tiny_obj_loader.h>
#include <Vertex.h>

// Load Model

class LoadModel {
public:
    LoadModel() = default;
    virtual  ~LoadModel() = default;

    [[nodiscard]] std::vector<Vertex>* get_vertices() { return &m_vertices; }
    [[nodiscard]] std::vector<uint32_t>* get_indices() { return &m_indices; }
    // void set_vertices(std::pmr::vector<Vertex>& vertices) { return &m_vertices; }
    // void set_indices(std::pmr::vector<Vertex>& vertices) { return &m_indices; }
private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
};

class LoadObjModel : public LoadModel {
public:
    explicit LoadObjModel(const char* obj_path) {
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
                    vertices->push_back(vertex);
                }

                indices->push_back(unique_vertices[vertex]);
            }
        }
    }
    ~LoadObjModel() = default;

    // [[nodiscard]] std::vector<Vertex>* get_vertices() { return &m_vertices; }
    // [[nodiscard]] std::vector<uint32_t>* get_indices() { return &m_indices; }

private:
    // std::vector<Vertex> m_vertices;
    // std::vector<uint32_t> m_indices;
};

class Model : public LoadObjModel {
public:
    enum Kind {
        VIKING_ROOM = 0,
        BAR,
    };

    explicit Model(const Kind kind) : LoadObjModel(get_obj_path(kind)) {
        m_kind = kind;
    }
    ~Model() = default;

    [[nodiscard]] Kind get_kind() const { return m_kind; }
private:
    Kind m_kind;

    static const char* get_obj_path(Kind kind) {
        switch (kind) {
            case VIKING_ROOM:
                return MODEL_PATH.c_str();
            case BAR:
                return "/Users/sebastian/CLionProjects/soundscape/models/bar.obj";
        }
        throw std::runtime_error("Unknown model kind");
    }
};

#endif //MODEL_H
