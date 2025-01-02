//
// Created by Sebastian Sandstig on 2024-12-06.
//

#ifndef MODEL_H
#define MODEL_H

#include <Vertex.h>

class LoadModel {
public:
    explicit LoadModel(const size_t vertex_size) : m_vertex_size(vertex_size) {}
    virtual  ~LoadModel() = default;

    [[nodiscard]] size_t get_vertex_size() const { return m_vertex_size; }
    [[nodiscard]] std::vector<char>* get_vertices() { return &m_vertices; }
    [[nodiscard]] std::vector<uint32_t>* get_indices() { return &m_indices; }

    template<typename T>
    void append_vertex(T& vertex) {
        const auto serialized_vertex = reinterpret_cast<char*>(&vertex);
        for (int i = 0; i < sizeof(vertex); i++) {
            m_vertices.push_back(serialized_vertex[i]);
        }
    }

    void append_index(const uint32_t index) {
        m_indices.push_back(index);
    }
private:
    size_t m_vertex_size;
    std::vector<char> m_vertices;
    std::vector<uint32_t> m_indices;
};

template <typename T>
class LoadObjModel final : public LoadModel {
public:
    explicit LoadObjModel(const char* obj_path);
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
                return "/Users/sebastian/CLionProjects/soundscape/models/viking_room.obj";
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

    static LoadModel* load_model(Kind kind);
};

#endif //MODEL_H
