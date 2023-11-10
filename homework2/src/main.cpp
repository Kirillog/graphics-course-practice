#include "glm/ext/matrix_float4x4.hpp"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_stdinc.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "camera.h"
#include "gl.h"
#include "mouse.h"
#include "obj_loader.h"
#include "stb_image.h"
#include "window.h"

const char *MAIN_PROGRAM = "src/shaders/main";
const char *SHADOW_PROGRAM = "src/shaders/shadow";

void DrawScene(GLuint program, GLuint &exists_alpha_texture, obj_data &data,
               std::vector<GLuint> &ambient_textures,
               std::vector<GLuint> &alpha_textures) {
    GLuint power_location = glGetUniformLocation(program, "power");
    GLuint glossiness_location = glGetUniformLocation(program, "glossiness");
    for (size_t i = 0; i < data.shape_indices.size(); ++i) {
        glActiveTexture(GL_TEXTURE1);
        GLuint ambient_texture =
        ambient_textures[data.shape_indices[i].material_index]; if
        (ambient_texture == GL_INVALID_INDEX) {
            continue;
        }
        glBindTexture(GL_TEXTURE_2D,
                      ambient_texture);
        auto [f, s, t] =
            data.materials[data.shape_indices[i].material_index].specular;
        glUniform3f(glossiness_location, f, s, t);
        glUniform1f(
            power_location,
            data.materials[data.shape_indices[i].material_index].shininess);

        GLuint alpha_texture =
            alpha_textures[data.shape_indices[i].material_index];
        if (alpha_texture == GL_INVALID_INDEX) {

            glUniform1i(exists_alpha_texture, 0);
        } else {
            glUniform1i(exists_alpha_texture, 1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, alpha_texture);
        }
        glDrawElements(
            GL_TRIANGLES, data.shape_indices[i].length, GL_UNSIGNED_INT,
            (void *) (data.shape_indices[i].offset * sizeof(std::uint32_t)));
    }
}
int main(int argc, char *argv[]) try {
    Window window;
    auto program = gl::CreateProgram(MAIN_PROGRAM);
    GLuint model_location = glGetUniformLocation(program, "model");
    GLuint view_location = glGetUniformLocation(program, "view");
    GLuint projection_location = glGetUniformLocation(program, "projection");
    GLuint camera_position_location =
        glGetUniformLocation(program, "camera_position");

    GLuint ambient_light_location =
        glGetUniformLocation(program, "ambient_light");
    GLuint sun_direction_location =
        glGetUniformLocation(program, "sun_direction");
    GLuint sun_color_location = glGetUniformLocation(program, "sun_color");
    GLuint point_light_position_location =
        glGetUniformLocation(program, "point_light_position");
    GLuint point_light_color_location =
        glGetUniformLocation(program, "point_light_color");
    GLuint point_light_attenuation_location =
        glGetUniformLocation(program, "point_light_attenuation");

    GLuint albedo_location = glGetUniformLocation(program, "albedo");

    GLuint ambient_sampler_location =
        glGetUniformLocation(program, "ambient_sampler");
    GLuint alpha_sampler_location =
        glGetUniformLocation(program, "alpha_sampler");
    GLuint exists_alpha_texture = glGetUniformLocation(program, "exists_alpha");

    GLuint main_shadow_projection_location =
        glGetUniformLocation(program, "shadow_projection");
    GLuint shadow_map_location = glGetUniformLocation(program, "shadow_map");

    auto shadow_program = gl::CreateProgram(SHADOW_PROGRAM);

    GLuint shadow_projection_location =
        glGetUniformLocation(shadow_program, "shadow_projection");
    GLuint shadow_model_location =
        glGetUniformLocation(shadow_program, "model");

    GLuint debug_program = gl::CreateProgram("src/shaders/debug");

    if (argc != 2) {
        std::cerr << "Incorrect number of arguments, 1 was expected\n";
        return 1;
    }

    std::string model_path = std::string(PROJECT_ROOT) + "/" + argv[1];

    obj_data data = parse_obj(model_path);

    GLuint debug_vao;
    glGenVertexArrays(1, &debug_vao);
    glBindVertexArray(debug_vao);

    GLuint model_vao, model_vbo, model_ebo;
    gl::GenBuffers(data, model_vao, model_vbo, model_ebo);

    GLuint shadow_map_texture, fbo;
    gl::GenShadowMap(shadow_map_texture, fbo);

    std::vector<GLuint> ambient_textures, alpha_textures;
    glActiveTexture(GL_TEXTURE1);
    gl::LoadTextures(data, ambient_textures, gl::MaterialTexture::AMBIENT);
    glActiveTexture(GL_TEXTURE2);
    gl::LoadTextures(data, alpha_textures, gl::MaterialTexture::ALPHA);

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    std::cerr << glGetError() << "\n";

    float time = 0.f;
    float radius = 100.f;

    std::map<SDL_Keycode, bool> button_down;

    Mouse m;
    Camera camera(m);

    bool running = true;
    while (running) {
        for (SDL_Event event; SDL_PollEvent(&event);)
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:
                    window.width = event.window.data1;
                    window.height = event.window.data2;
                    break;
                }
                break;
            case SDL_KEYDOWN:
                button_down[event.key.keysym.sym] = true;
                break;
            case SDL_KEYUP:
                button_down[event.key.keysym.sym] = false;
                break;
            case SDL_MOUSEMOTION:
                m.xpos = event.motion.x;
                m.ypos = event.motion.y;
                break;
            }

        if (!running)
            break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(
                       now - last_frame_start)
                       .count();
        last_frame_start = now;
        time += dt;

        camera.Move(button_down, dt);

        // glEnable(GL_BLEND);
        // glBlendEquation(GL_FUNC_ADD);
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float near = 0.1f;
        float far = 100000.f;

        glm::mat4 model(1.f);

        glm::mat4 view = camera.GenView();

        float aspect = (float) window.height / (float) window.width;
        glm::mat4 projection =
            glm::perspective(glm::pi<float>() / 3.f,
                             (window.width * 1.f) / window.height, near, far);

        glm::vec4 temp = (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f));

        glm::vec3 camera_position = glm::vec3(temp.x, temp.y, temp.z);

        glm::vec3 sun_direction = glm::normalize(
            glm::vec3(std::sin(time * 0.5f), 2.f, std::cos(time * 0.5f)));

        auto light_Z = -sun_direction;
        auto light_X = glm::normalize(
            glm::vec3(std::sin(time * 0.5f), -0.5f, std::cos(time * 0.5f)));
        auto light_Y = glm::cross(light_X, light_Z);
        auto shadow_projection =
            glm::mat4(glm::transpose(glm::mat3(light_X, light_Y, light_Z))) *
            0.0005f;
        shadow_projection[3][3] = 1.f;

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glViewport(0, 0, gl::SHADOW_MAP_SIZE, gl::SHADOW_MAP_SIZE);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);

        glUseProgram(shadow_program);

        glUniformMatrix4fv(shadow_model_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(shadow_projection_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&shadow_projection));

        glBindVertexArray(model_vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, shadow_map_texture);

        glDrawElements(GL_TRIANGLES, data.vertex_indices.size(),
                       GL_UNSIGNED_INT, (void *) (0));
        glCullFace(GL_BACK);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, window.width, window.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.8f, 0.8f, 1.f, 0.f);

        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        glUseProgram(program);

        glUniformMatrix4fv(model_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(view_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&projection));
        glUniformMatrix4fv(main_shadow_projection_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&shadow_projection));
        glUniform3fv(camera_position_location, 1, (float *) (&camera_position));

        glUniform3f(ambient_light_location, 0.2f, 0.2f, 0.2f);
        glUniform3f(albedo_location, .8f, .7f, .6f);
        glUniform3f(sun_color_location, 1.f, 1.f, 1.f);
        glUniform3fv(sun_direction_location, 1,
                     reinterpret_cast<float *>(&sun_direction));

        glUniform3f(point_light_color_location, 0.f, 1.f, 0.f);
        glUniform3f(point_light_position_location, radius * cos(time), 0.f,
                    radius * sin(time));
        glUniform3f(point_light_attenuation_location, 1.0f, 0.0f, 0.01f);

        glUniform1i(ambient_sampler_location, 1);
        glUniform1i(alpha_sampler_location, 2);

        glBindVertexArray(model_vao);

        std::cerr << "\t" << glGetError() << "\n";

        DrawScene(program, exists_alpha_texture, data, ambient_textures,
          alpha_textures);

        // glDrawElements(GL_TRIANGLES, data.vertex_indices.size(),
                    //    GL_UNSIGNED_INT, (void *) (0));

        std::cerr << "\t" << glGetError() << "\n";

        glUseProgram(debug_program);
        glDisable(GL_DEPTH_TEST);

        glBindVertexArray(debug_vao);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_DEPTH_TEST);
        glBindTexture(GL_TEXTURE_2D, shadow_map_texture);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        SDL_GL_SwapWindow(window.window);
    }
} catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
