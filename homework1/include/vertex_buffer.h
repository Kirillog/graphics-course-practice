#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#include <GL/glew.h>

#include <vector>
#include <cstdint>

template <typename T> struct vertex_buffer {
    std::vector<T> vertices;
    std::vector<std::uint32_t> indices;

    void load_data(GLuint vao, GLuint vbo, GLuint ebo);
};

template <typename T>
void vertex_buffer<T>::load_data(GLuint vao, GLuint vbo, GLuint ebo) {
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(T) * vertices.size(), vertices.data(),
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(std::uint32_t) * indices.size(), indices.data(),
                 GL_DYNAMIC_DRAW);
}
#endif