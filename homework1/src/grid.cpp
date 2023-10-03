#include "grid.h"
#include "perlin.h"

vertex_buffer<vec2> Grid::gen_vertices(int width, int height, int w_count,
                                       int h_count) {

    int img_height = 0.8 * height, img_width = 0.8 * width;
    if (img_height < img_width) {
        img_width = img_height;
    } else {
        img_height = img_width;
    }

    float bottom = 0.5 * (height - img_height),
          up = 0.5 * (height + img_height);
    float left = 0.5 * (width - img_width), right = 0.5 * (width + img_width);
    vertex_index = std::vector<std::vector<std::uint32_t>>(
        h_count + 1, std::vector<std::uint32_t>(w_count + 1));
    vertex_pos = std::vector<std::vector<vec2>>(h_count + 1,
                                                std::vector<vec2>(w_count + 1));
    size_t index = 0;
    for (int y = 0; y <= h_count; ++y) {
        for (int x = 0; x <= w_count; ++x, ++index) {
            vertex_index[y][x] = index;
            vertex_pos[y][x] = {left + x * (right - left) / w_count,
                                bottom + y * (up - bottom) / h_count};
        }
    }
    std::vector<std::uint32_t> indices;
    for (int y = 0; y < h_count; ++y) {
        for (int x = 0; x < w_count; ++x) {
            indices.push_back(vertex_index[y][x]);
            indices.push_back(vertex_index[y + 1][x]);
            indices.push_back(vertex_index[y + 1][x + 1]);
            indices.push_back(vertex_index[y + 1][x + 1]);
            indices.push_back(vertex_index[y][x + 1]);
            indices.push_back(vertex_index[y][x]);
        }
    }
    return {flat(vertex_pos), indices};
}

std::vector<color_t> Grid::calc_function(float time) {
    vertex_color = std::vector<std::vector<color_t>>(
        vertex_pos.size(), std::vector<color_t>(vertex_pos[0].size()));

    for (int y = 0; y < vertex_pos.size(); ++y) {
        for (int x = 0; x < vertex_pos[y].size(); ++x) {
            float noise_value = perlin(vertex_pos[y][x].x / 100,
                                       vertex_pos[y][x].y / 100, time);
            vertex_color[y][x] = {noise_value, noise_value, noise_value, 1.0};
        }
    }

    return flat(vertex_color);
}

void IsolineMap::insert(EdgeIndex ind, vec2 point) {
    if (edge_indices.find(ind) == edge_indices.end()) {
        edge_indices[ind] = vertices.size();
        vertices.push_back({point, {255, 0, 0, 255}});
    }
    indices.push_back(edge_indices[ind]);
}

void Grid::add_intersect_points(IsolineMap &map, weighted_vertex a,
                                weighted_vertex b, weighted_vertex c,
                                float constant) {
    std::vector<weighted_vertex> verts = {a, b, c};
    std::vector<weighted_vertex> less_verts, bigger_verts;
    std::copy_if(verts.begin(), verts.end(), std::back_inserter(less_verts),
                 [=](const weighted_vertex &v) { return v.value < constant; });
    std::copy_if(verts.begin(), verts.end(), std::back_inserter(bigger_verts),
                 [=](const weighted_vertex &v) { return v.value >= constant; });
    if (less_verts.empty() || bigger_verts.empty()) {
        return;
    }
    if (less_verts.size() > bigger_verts.size()) {
        std::swap(less_verts, bigger_verts);
    }
    std::vector<edge> crossed_edges = {edge(less_verts[0], bigger_verts[0]),
                                       edge(less_verts[0], bigger_verts[1])};

    for (const edge &e : crossed_edges) {
        auto [v1, v2] = e;
        float alpha = (constant - v2.value) / (v1.value - v2.value);
        vec2 intersect_point = {
            alpha * v1.position.x + (1 - alpha) * v2.position.x,
            alpha * v1.position.y + (1 - alpha) * v2.position.y};

        EdgeIndex ind = {v1.index, v2.index};
        map.insert(ind, intersect_point);
    }
}

vertex_buffer<vertex> Grid::calc_isolines(const std::vector<float> &constants) {
    auto at = [&](int x, int y) -> weighted_vertex {
        return {vertex_pos[y][x], vertex_index[y][x], vertex_color[y][x][0]};
    };

    IsolineMap map;
    for (int y = 0; y + 1 < vertex_pos.size(); ++y) {
        for (int x = 0; x + 1 < vertex_pos[y].size(); ++x) {
            weighted_vertex lu = at(x, y), ru = at(x + 1, y), ld = at(x, y + 1),
                            rd = at(x + 1, y + 1);
            /* lu ---- ru
             * | \     |
             * |  \    |
             * |   \   |
             * |    \  |
             * |     \ |
             * |      \|
             * ld ---- rd
             */
            for (auto &c : constants) {
                add_intersect_points(map, lu, ld, rd, c);
                add_intersect_points(map, lu, rd, ru, c);
            }
        }
    }
    return {map.vertices, map.indices};
}
