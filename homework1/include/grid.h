#ifndef GRID_H
#define GRID_H

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "vertex_buffer.h"

typedef std::array<float, 4> color_t;
typedef std::uint32_t index_t;

const float eps = 1e-6;

struct vec2 {
    float x, y;
};

struct weighted_vertex {
    vec2 position;
    index_t index;
    float value;
};


struct vertex {
    vec2 position;
    color_t color;
};

struct edge {
    weighted_vertex f, s;

    edge() = default;

    edge(weighted_vertex left, weighted_vertex right) {
        if (left.position.x + eps < right.position.x ||
            right.position.x - left.position.x <= eps &&
                left.position.y + eps < right.position.y) {
            f = left;
            s = right;
        } else {
            f = right;
            s = left;
        }
    }
};

typedef std::pair<int, int> EdgeIndex;

struct PairHash {
    size_t operator()(const EdgeIndex &p) const {
        return std::hash<int>{}(p.first) ^ std::hash<int>{}(p.second);
    }
};

struct IsolineMap {
    std::vector<index_t> indices;
    std::vector<vertex> vertices;
    std::unordered_map<EdgeIndex, index_t, PairHash> edge_indices;

    void insert(EdgeIndex ind, vec2 point);
};

struct Grid {
    std::vector<std::vector<vec2>> vertex_pos;
    std::vector<std::vector<index_t>> vertex_index;
    std::vector<std::vector<color_t>> vertex_color;

    vertex_buffer<vec2> gen_vertices(int width, int height, int w_count,
                                          int h_count);
    std::vector<color_t> calc_function(float time);
    void add_intersect_points(IsolineMap &map, weighted_vertex a,
                              weighted_vertex b, weighted_vertex c,
                              float constant);
    vertex_buffer<vertex> calc_isolines(const std::vector<float>& constants);
};

template <typename T> std::vector<T> flat(std::vector<std::vector<T>> &vector) {
    std::vector<T> flat_vector;
    for (auto &i : vector) {
        for (auto &j : i) {
            flat_vector.push_back(j);
        }
    }
    return flat_vector;
}

#endif