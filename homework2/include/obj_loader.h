#pragma once

#include "tiny_obj_loader.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <vector>

#define MODEL_DIR "/sponza"

struct obj_data {
    using index = std::uint32_t;

    struct vertex {
        std::array<float, 3> position;
        std::array<float, 3> normal;
        std::array<float, 2> texcoord;
    };

    struct span {
        index offset;
        index length;
        int material_index;
    };

    std::vector<vertex> vertices;
    std::vector<index> vertex_indices;
    std::vector<span> shape_indices;
    std::vector<tinyobj::material_t> materials;
};

struct cmp {
    bool operator()(const tinyobj::index_t &a,
                    const tinyobj::index_t &b) const {
        return std::make_tuple(a.vertex_index, a.normal_index,
                               a.texcoord_index) <
               std::make_tuple(b.vertex_index, b.normal_index,
                               b.texcoord_index);
    }
};

template <typename T> struct indexed_container {
    std::vector<T> underlying;
    std::vector<obj_data::index> indices;
    std::map<tinyobj::index_t, obj_data::index, cmp> index_map;

    obj_data::index push_back(tinyobj::index_t index, T element) {
        auto it = index_map.find(index);
        if (it != index_map.end()) {
            indices.push_back(it->second);
            return it->second;
        } else {
            obj_data::index result_index = underlying.size();
            underlying.push_back(element);
            index_map[index] = result_index;
            indices.push_back(result_index);
            return result_index;
        }
    }
};

obj_data parse_obj(std::filesystem::path const &path);