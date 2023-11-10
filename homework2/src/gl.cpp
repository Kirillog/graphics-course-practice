#include "gl.h"

const char vertex_shader_source[] =
    R"(#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

out vec3 position;
out vec3 normal;
out vec2 texcoord;

void main()
{
    position = (model * vec4(in_position, 1.0)).xyz;
    gl_Position = projection * view * vec4(position, 1.0);
    normal = normalize(mat3(model) * in_normal);
    texcoord = vec2(in_texcoord.x, 1 - in_texcoord.y);
}
)";

const char fragment_shader_source[] =
    R"(#version 330 core

uniform vec3 camera_position;

uniform vec3 ambient_light;

uniform vec3 sun_direction;
uniform vec3 sun_color;

uniform vec3 point_light_position;
uniform vec3 point_light_color;
uniform vec3 point_light_attenuation;

uniform float glossiness;
uniform float roughness;

// uniform mat4 shadow_projection;
// uniform sampler2DShadow shadow_map;
uniform sampler2D ambient_sampler;
uniform sampler2D alpha_sampler;
uniform int exists_alpha;

in vec3 position;
in vec3 normal;
in vec2 texcoord;

layout (location = 0) out vec4 out_color;

vec3 diffuse(vec3 direction, vec3 albedo) {
    return albedo * max(0.0, dot(normal, normalize(direction)));
}

vec3 specular(vec3 direction, vec3 albedo) {
    float power = 1.0 / (roughness * roughness) - 1;
    vec3 light_direction = normalize(direction);
    vec3 reflected = 2.0 * normal * dot(normal, light_direction) - light_direction;
    vec3 view_direction = camera_position - position;
    return glossiness * albedo * pow(max(0.0, dot(reflected, normalize(view_direction))), power);
}

vec3 phong(vec3 direction, vec3 albedo) {
    return diffuse(direction, albedo) + specular(direction, albedo);
}

void main()
{
    vec2 cord = vec2(texcoord.x, texcoord.y);
    vec4 data = texture(ambient_sampler, cord);
    vec3 albedo = vec3(data.x, data.y, data.z);

    vec3 ambient = albedo * ambient_light;
    vec3 point_light_direction = point_light_position - position;
    float point_light_dist = length(point_light_direction);
    float attenuation_coef = 1.0 / (point_light_attenuation.x + point_light_dist * point_light_attenuation.y + point_light_dist * point_light_dist * point_light_attenuation.z);
    vec3 color = ambient + phong(point_light_direction, albedo) * point_light_color * attenuation_coef;

    /*vec4 ndc = shadow_projection * model * vec4(position, 1);
    if (abs(ndc.x) <= 1 && abs(ndc.y) <= 1) {
        vec2 shadow_texcoord = ndc.xy * 0.5 + 0.5;
        float shadow_depth = ndc.z * 0.5 + 0.5;

        float sum = 0.0;
        float sum_w = 0.0;
        const int N = 5;
        float radius = 5.0;
        for (int x = -N; x <= N; ++x) {
            for (int y = -N; y <= N; ++y) {
                float c = exp(-float(x * x + y * y) / (radius * radius));
                sum += c * texture(shadow_map, vec3(shadow_texcoord + vec2(x,y) / vec2(textureSize(shadow_map, 0)), shadow_depth));
                sum_w += c;
            }
        }
        color += phong(sun_direction, albedo) * sun_color * sum / sum_w;
    } else {*/
        color += phong(sun_direction, albedo) * sun_color;
    //}

    float alpha = 0.0;
    if (exists_alpha == 1) {
        if (texture(alpha_sampler, cord).w < 0.5) {
            discard;
        } else {
            alpha = texture(alpha_sampler, cord).w;
        }
    } else {
        alpha = data.w;
    }
    out_color = vec4(color, alpha);
}
)";

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

GLuint create_vertex_shader() {
    return create_shader(GL_VERTEX_SHADER, vertex_shader_source);
}

GLuint create_fragment_shader() {
    return create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
}

GLuint create_program() {
    GLuint result = glCreateProgram();
    glAttachShader(result, create_vertex_shader());
    glAttachShader(result, create_fragment_shader());
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
