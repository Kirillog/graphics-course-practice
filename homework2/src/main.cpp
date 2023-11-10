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

int main() try {
    Window window;
    auto program = create_program();
    GLuint model_location = glGetUniformLocation(program, "model");
    GLuint view_location = glGetUniformLocation(program, "view");
    GLuint projection_location = glGetUniformLocation(program, "projection");
    GLuint camera_position_location =
        glGetUniformLocation(program, "camera_position");
    GLuint albedo_location = glGetUniformLocation(program, "albedo");
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
    GLuint roughness_location = glGetUniformLocation(program, "roughness");
    GLuint glossiness_location = glGetUniformLocation(program, "glossiness");
    GLuint ambient_sampler_location =
        glGetUniformLocation(program, "ambient_sampler");
    GLuint alpha_sampler_location =
        glGetUniformLocation(program, "alpha_sampler");
    GLuint exists_alpha_texture = glGetUniformLocation(program, "exists_alpha");

    std::string project_root = PROJECT_ROOT;
    std::string model_path = project_root + MODEL_DIR + "/sponza.obj";

    obj_data data = parse_obj(model_path);

    GLuint model_vao, model_vbo, model_ebo;
    GenBuffers(data, model_vao, model_vbo, model_ebo);

    std::vector<GLuint> ambient_textures, alpha_textures;
    glActiveTexture(GL_TEXTURE1);
    LoadTextures(data, ambient_textures, MaterialTexture::AMBIENT);
    glActiveTexture(GL_TEXTURE2);
    LoadTextures(data, alpha_textures, MaterialTexture::ALPHA);

    auto last_frame_start = std::chrono::high_resolution_clock::now();

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

        glViewport(0, 0, window.width, window.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.8f, 0.8f, 1.f, 0.f);

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        float near = 0.1f;
        float far = 100000.f;

        glm::mat4 model(1.f);

        glm::mat4 view = camera.GenView();

        camera.Move(button_down, dt);

        float aspect = (float) window.height / (float) window.width;
        glm::mat4 projection =
            glm::perspective(glm::pi<float>() / 3.f,
                             (window.width * 1.f) / window.height, near, far);

        glm::vec3 camera_position =
            (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f)).xyz();

        glUseProgram(program);

        glUniformMatrix4fv(view_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&projection));
        glUniform3fv(camera_position_location, 1, (float *) (&camera_position));
        glUniform3f(albedo_location, 0.7f, 0.4f, 0.2f);
        glUniform3f(ambient_light_location, 0.2f, 0.2f, 0.2f);
        glUniform3f(sun_color_location, 1.0f, 0.9f, 0.8f);
        glUniform3f(sun_direction_location, 0.0f, 0.1f, 1.0f);
        glUniform3f(point_light_color_location, 0.f, 1.f, 0.f);
        glUniform3f(point_light_position_location, radius * cos(time), 0.f,
                    radius * sin(time));
        glUniform3f(point_light_attenuation_location, 1.0f, 0.0f, 0.01f);
        glUniform1i(ambient_sampler_location, 1);
        glUniform1i(alpha_sampler_location, 2);

        glBindVertexArray(model_vao);
        glUniformMatrix4fv(model_location, 1, GL_FALSE,
                           reinterpret_cast<float *>(&model));
        glUniform1f(glossiness_location, static_cast<float>(5));
        glUniform1f(roughness_location, 0.1f);

        for (size_t i = 0; i < data.shape_indices.size(); ++i) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(
                GL_TEXTURE_2D,
                ambient_textures[data.shape_indices[i].material_index]);

            GLuint alpha_texture =
                alpha_textures[data.shape_indices[i].material_index];
            if (alpha_texture == GL_INVALID_INDEX) {
                glUniform1i(exists_alpha_texture, 0);
            } else {
                glUniform1i(exists_alpha_texture, 1);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, alpha_texture);
            }

            glDrawElements(GL_TRIANGLES, data.shape_indices[i].length,
                           GL_UNSIGNED_INT,
                           (void *) (data.shape_indices[i].offset *
                                     sizeof(std::uint32_t)));
        }
        SDL_GL_SwapWindow(window.window);
    }
} catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
