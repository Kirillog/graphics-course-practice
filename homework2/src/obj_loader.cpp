#include "obj_loader.h"

obj_data parse_obj(std::filesystem::path const &path) {
    std::string inputfile = path.string();
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path =
        std::string(PROJECT_ROOT) + MODEL_DIR;   // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(inputfile, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();
    auto &materials = reader.GetMaterials();
    indexed_container<obj_data::vertex> vertex_indices;

    obj_data data;
    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {


        size_t index_offset = 0;
        // Loop over faces(polygon)
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            obj_data::span shape;
            shape.offset = data.vertex_indices.size();
            shape.length = 0;
            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                obj_data::vertex vertex;
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                float vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
                float vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
                float vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
                vertex.position = {vx, vy, vz};



                if (idx.normal_index >= 0) {
                    float nx = attrib.normals[3*size_t(idx.normal_index)+0];
                    float ny = attrib.normals[3*size_t(idx.normal_index)+1];
                    float nz = attrib.normals[3*size_t(idx.normal_index)+2];
                    vertex.normal = {nx, ny, nz};
                }

                if (idx.texcoord_index >= 0) {
                    float tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
                    float ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
                    vertex.texcoord = {tx, ty};
                }

                auto index_in_my_data = vertex_indices.push_back(idx, vertex);
                data.vertex_indices.push_back(index_in_my_data);
                ++shape.length;
            }
            index_offset += fv;

            // per-face material
            int material_index = shapes[s].mesh.material_ids[f];
            shape.material_index = material_index;
            data.shape_indices.push_back(shape);
        }
    }
    data.vertices = vertex_indices.underlying;
    data.materials = materials;
    return data;
}