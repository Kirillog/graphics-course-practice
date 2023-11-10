#include "gl.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace gl {

GLuint create_shader(GLenum type, const char *source) {
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, nullptr);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint info_log_length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Shader compilation failed: " + info_log);
    }
    return result;
}

std::string load_shader(const std::filesystem::path &shader_source) {
    std::ifstream in(shader_source.string());
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

GLuint create_vertex_shader(const std::filesystem::path &vertex_shader_source) {
    return create_shader(GL_VERTEX_SHADER,
                         load_shader(vertex_shader_source).c_str());
}

GLuint
create_fragment_shader(const std::filesystem::path &fragment_shader_source) {
    return create_shader(GL_FRAGMENT_SHADER,
                         load_shader(fragment_shader_source).c_str());
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint result = glCreateProgram();
    glAttachShader(result, vertex_shader);
    glAttachShader(result, fragment_shader);
    glLinkProgram(result);

    GLint status;
    glGetProgramiv(result, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint info_log_length;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }

    return result;
}

GLuint CreateProgram(const std::string &prefix) {
    std::string vertex_shader_name = prefix + ".vert";
    std::string fragment_shader_name = prefix + ".frag";
    GLuint vertex_shader = create_vertex_shader(vertex_shader_name);
    GLuint fragment_shader = create_fragment_shader(fragment_shader_name);
    return create_program(vertex_shader, fragment_shader);
}

void GenBuffers(obj_data &data, GLuint &model_vao, GLuint &model_vbo,
                GLuint &model_ebo) {
    glGenVertexArrays(1, &model_vao);
    glBindVertexArray(model_vao);

    glGenBuffers(1, &model_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, model_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 data.vertices.size() * sizeof(data.vertices[0]),
                 data.vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &model_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 data.vertex_indices.size() * sizeof(data.vertex_indices[0]),
                 data.vertex_indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex),
                          (void *) (offsetof(obj_data::vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex),
                          (void *) (offsetof(obj_data::vertex, normal)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(obj_data::vertex),
                          (void *) (offsetof(obj_data::vertex, texcoord)));
}

void LoadTextures(const obj_data &data, std::vector<GLuint> &textures,
                  MaterialTexture tex_type) {
    for (size_t i = 0; i < data.materials.size(); ++i) {

        size_t x, y;
        const tinyobj::material_t &material = data.materials[i];
        std::string texture_path = PROJECT_ROOT + std::string(MODEL_DIR) + "/";
        switch (tex_type) {
        case MaterialTexture::AMBIENT:
            texture_path += material.ambient_texname;
            break;
        case MaterialTexture::ALPHA:
            texture_path += material.alpha_texname;
            break;
        default:
            assert(false);
        }
        std::replace(texture_path.begin(), texture_path.end(), '\\', '/');

        unsigned char *texture_data =
            stbi_load(texture_path.data(), reinterpret_cast<int *>(&x),
                      reinterpret_cast<int *>(&y), nullptr, 4);
        if (texture_data == nullptr) {
            std::cerr << "Cannot load " << texture_path << "\n";
            textures.push_back(GL_INVALID_INDEX);
        } else {
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, texture_data);
            glGenerateMipmap(GL_TEXTURE_2D);
            textures.push_back(texture);
        }

        stbi_image_free(texture_data);
    }
}

void GenShadowMap(GLuint &shadow_map_texture, GLuint &fbo) {
    glGenTextures(1, &shadow_map_texture);
    glBindTexture(GL_TEXTURE_2D, shadow_map_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_MAP_SIZE,
                 SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
                    GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                         shadow_map_texture, 0);
    assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) ==
           GL_FRAMEBUFFER_COMPLETE);
}

}   // namespace gl
