//
// Created by Sebastian Sandstig on 2025-01-02.
//


#include <Model.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

template <typename T>
LoadObjModel<T>::LoadObjModel(const char* obj_path) : LoadModel(sizeof(T)){
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
        }
    }

    auto vertices = object.get_vertices<T>();
    for (size_t i = 0; i < vertices.size(); i++) {
        append_vertex(vertices[i]);
    }
    for (auto index : object.get_indices()) {
        append_index(index);
    }
}


LoadModel* Model::load_model(Model::Kind kind) {
    switch (kind) {
        case Model::VIKING_ROOM:
            return new LoadObjModel<StandardVertex>(get_obj_path(kind));
        case Model::BAR:
            return new LoadObjModel<BarVertex>(get_obj_path(kind));
        case Model::BACK_DROP:
            return new LoadObjModel<BackDropVertex>(get_obj_path(kind));
        case Model::COVER_ART:
            return new LoadObjModel<CoverArtVertex>(get_obj_path(kind));
    }
    throw std::runtime_error("Unknown model kind");
}