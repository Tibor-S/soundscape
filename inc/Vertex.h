//
// Created by Sebastian Sandstig on 2024-12-03.
//

#ifndef VERTEX_H
#define VERTEX_H

#include <set>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>


class ObjVertex {
public:
    struct Unique {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 tex_coord;

        bool operator==(const Unique& other) const {
            return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
        }
    };


    explicit ObjVertex(Unique data) : m_data(data) {}
    ~ObjVertex() = default;

    void add_normal(const glm::vec3& normal_vector) {
        m_vertex_normal += normal_vector;
    }
    void append_vertex_group(size_t group) {
        m_vertex_groups.insert(group);
    }

    [[nodiscard]] glm::vec3 get_pos() const { return m_data.pos; }
    [[nodiscard]] glm::vec3 get_color() const { return m_data.color; }
    [[nodiscard]] glm::vec2 get_tex_coord() const { return m_data.tex_coord; }
    [[nodiscard]] glm::vec3 get_normal() const { return glm::normalize(m_vertex_normal); }
    [[nodiscard]] std::vector<size_t> get_vertex_groups() const { return {m_vertex_groups.begin(), m_vertex_groups.end()}; }

private:
    // Unique parameters per vertex
    Unique m_data;
    // Combined parameters averaged and appended when unique parameters match
    glm::vec3 m_vertex_normal = glm::vec3(0.0f);
    std::pmr::set<size_t> m_vertex_groups;
};

template<> struct std::hash<ObjVertex::Unique> {
    size_t operator()(ObjVertex::Unique const& data) const noexcept {
        return (hash<glm::vec3>()(data.pos) ^
            (hash<glm::vec3>()(data.color) << 1) >> 1) ^
                (hash<glm::vec2>()(data.tex_coord) << 1);
    }
};

class Object3D {
public:

    Object3D() = default;
    ~Object3D() = default;

    void add_face(const std::array<ObjVertex::Unique, 3> &face, const glm::vec3 face_normal, size_t vertex_group) {

        for (const auto& data : face) {
            if (!m_vertex_index.contains(data)) {
                m_vertex_index[data] = m_vertex_index.size();
                m_vertices.emplace_back(data);
            }

            const auto index = m_vertex_index[data];
            m_vertices[index].add_normal(face_normal);
            m_vertices[index].append_vertex_group(vertex_group);

            m_indices.push_back(index);
        }
    }

    template<typename T>
    [[nodiscard]] std::vector<T> get_vertices() {
        std::vector<T> return_vector(m_vertices.size());
        for (size_t i = 0; i < m_vertices.size(); i++) {
            return_vector[i] = T(m_vertices[i]);
        }
        return return_vector;
    }
    [[nodiscard]] std::vector<u_int32_t> get_indices() { return m_indices; }

private:
    std::unordered_map<ObjVertex::Unique, size_t> m_vertex_index;
    std::vector<ObjVertex> m_vertices;
    std::vector<u_int32_t> m_indices;
};

struct StandardVertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 tex_coord;

    bool operator==(const StandardVertex& other) const {
        return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
    }

    StandardVertex(): pos(), color(), tex_coord() {}
    explicit StandardVertex(const ObjVertex &src): pos(src.get_pos()), color(src.get_color()),
                                                   tex_coord(src.get_tex_coord()) {}

    static VkVertexInputBindingDescription get_binding_description() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(StandardVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions() {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions(3);

        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(StandardVertex, pos);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(StandardVertex, color);

        attribute_descriptions[2].binding = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(StandardVertex, tex_coord);

        return attribute_descriptions;
    }
};

template<> struct std::hash<StandardVertex> {
    size_t operator()(StandardVertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.tex_coord) << 1);
    }
};

struct BarVertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;
    glm::int32 v_group = -1;

    bool operator==(const BarVertex& other) const {
        return pos == other.pos && color == other.color && normal == other.normal && v_group == other.v_group;
    }

    BarVertex(): pos(), color(), normal(), v_group() {}
    explicit BarVertex(const ObjVertex &src): pos(src.get_pos()), color(src.get_color()), normal(src.get_normal())
    {
        auto v_groups = src.get_vertex_groups();
        if (!v_groups.empty()) {
            v_group = static_cast<glm::int32>(v_groups[0]);
        }
        // auto len = v_groups.size();
        // if (1 > len) len = 1;
        // for (size_t i = 0; i < len; i++) {
        //     v_groups[i] = v_groups[i];
        // }
    }


    static VkVertexInputBindingDescription get_binding_description() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(BarVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions() {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions(4);

        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(BarVertex, pos);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(BarVertex, color);

        attribute_descriptions[2].binding = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(BarVertex, normal);

        attribute_descriptions[3].binding = 0;
        attribute_descriptions[3].location = 3;
        attribute_descriptions[3].format = VK_FORMAT_R32_SINT;
        attribute_descriptions[3].offset = offsetof(BarVertex, v_group);

        return attribute_descriptions;
    }
};

template<> struct std::hash<BarVertex> {
    size_t operator()(BarVertex const& vertex) const noexcept {
        return ((hash<glm::vec3>()(vertex.pos) ^
            (hash<glm::vec3>()(vertex.color) << 1) >> 1) ^
                (hash<glm::vec3>()(vertex.normal) << 1) >> 1) ^
                    (hash<glm::int32>()(vertex.v_group) << 1);
    }
};

struct BackDropVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 tex_coord;

    bool operator==(const BackDropVertex& other) const {
        return pos == other.pos && normal == other.normal && tex_coord == other.tex_coord;
    }


    BackDropVertex(): pos(), normal(), tex_coord() {}
    explicit BackDropVertex(const ObjVertex &src): pos(src.get_pos()), normal(src.get_normal()),
                                                   tex_coord(src.get_tex_coord()) {}

    static VkVertexInputBindingDescription get_binding_description() {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = 0;
        binding_description.stride = sizeof(BackDropVertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions() {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions(3);

        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(BackDropVertex, pos);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(BackDropVertex, normal);

        attribute_descriptions[2].binding = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(BackDropVertex, tex_coord);

        return attribute_descriptions;
    }
};

template<> struct std::hash<BackDropVertex> {
    size_t operator()(BackDropVertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^
                 (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.tex_coord) << 1);
    }
};

struct CoverArtVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 tex_coord;


    CoverArtVertex(): pos(), normal(), tex_coord() {}
    explicit CoverArtVertex(const ObjVertex &src): pos(src.get_pos()), normal(src.get_normal()),
                                                   tex_coord(src.get_tex_coord()) {}

    static VkVertexInputBindingDescription get_binding_description() {
        VkVertexInputBindingDescription binding_description{};
        binding_description.binding = 0;
        binding_description.stride = sizeof(CoverArtVertex);
        binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return binding_description;
    }

    static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions() {
        std::vector<VkVertexInputAttributeDescription> attribute_descriptions(3);

        attribute_descriptions[0].binding = 0;
        attribute_descriptions[0].location = 0;
        attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[0].offset = offsetof(CoverArtVertex, pos);

        attribute_descriptions[1].binding = 0;
        attribute_descriptions[1].location = 1;
        attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribute_descriptions[1].offset = offsetof(CoverArtVertex, normal);

        attribute_descriptions[2].binding = 0;
        attribute_descriptions[2].location = 2;
        attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attribute_descriptions[2].offset = offsetof(CoverArtVertex, tex_coord);

        return attribute_descriptions;
    }
};

#endif //VERTEX_H
