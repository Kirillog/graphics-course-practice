#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "gl.h"
#include "grid.h"
#include "perlin.h"
#include "window.h"

int main() try {
    Window window;
    auto program = create_program();
    GLuint view_location = glGetUniformLocation(program, "view");

    GLuint vao_grid, vao_isolines, ebo_isolines, vbo_isolines,
        vbo_grid_position, vbo_grid_color, ebo_grid;
    glGenVertexArrays(1, &vao_grid);
    glBindVertexArray(vao_grid);

    glGenBuffers(1, &vbo_grid_position);
    glGenBuffers(1, &vbo_grid_color);
    glGenBuffers(1, &ebo_grid);

    int cell_count = 100;
    int isolines_count = 1;

    std::vector<float> isoline_constants = {0.5};

    Grid grid;
    auto buffer =
        grid.gen_vertices(window.width, window.height, cell_count, cell_count);
    buffer.load_data(vao_grid, vbo_grid_position, ebo_grid);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_grid_position);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_grid_color);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, 0, nullptr);

    glGenVertexArrays(1, &vao_isolines);

    glGenBuffers(1, &vbo_isolines);
    glGenBuffers(1, &ebo_isolines);

    glBindVertexArray(vao_isolines);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_isolines);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),
                          (void *) offsetof(vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(vertex),
                          (void *) offsetof(vertex, color));

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    bool running = true;
    bool redraw = false;
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
                    glViewport(0, 0, window.width, window.height);
                    const GLfloat view[16] = {2.f / window.width,
                                              0,
                                              0,
                                              -1.f,
                                              0,
                                              -2.f / window.height,
                                              0,
                                              1.f,
                                              0,
                                              0,
                                              1,
                                              0,
                                              0,
                                              0,
                                              0,
                                              1};
                    glUniformMatrix4fv(view_location, 1, GL_TRUE, view);
                    redraw = true;
                    break;
                }
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_LEFT) {
                    if (isolines_count > 0) {
                        isolines_count -= 1;
                    }
                } else if (event.key.keysym.sym == SDLK_RIGHT) {
                    if (isolines_count < 10) {
                        isolines_count += 1;
                    }
                } else if (event.key.keysym.sym == SDLK_UP) {
                    if (cell_count < 500) {
                        cell_count += 50;
                    }
                    redraw = true;
                } else if (event.key.keysym.sym == SDLK_DOWN) {
                    if (cell_count > 50) {
                        cell_count -= 50;
                    }
                    redraw = true;
                }
                isoline_constants = std::vector<float>(isolines_count);
                for (int i = 1; i <= isolines_count; i++) {
                    isoline_constants[i - 1] = i * 1. / (isolines_count + 1);
                }
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

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        if (redraw) {
            buffer = grid.gen_vertices(window.width, window.height,
                                            cell_count, cell_count);
            buffer.load_data(vao_grid, vbo_grid_position, ebo_grid);
            redraw = false;
        }

        auto colors = grid.calc_function(time);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_grid_color);
        glBufferData(GL_ARRAY_BUFFER, sizeof(color_t) * colors.size(),
                     colors.data(), GL_DYNAMIC_DRAW);
        auto isolines = grid.calc_isolines(isoline_constants);
        isolines.load_data(vao_isolines, vbo_isolines, ebo_isolines);

        glBindVertexArray(vao_grid);
        glDrawElements(GL_TRIANGLES, buffer.indices.size(), GL_UNSIGNED_INT,
                       (void *) 0);

        glBindVertexArray(vao_isolines);
        glDrawElements(GL_LINES, isolines.indices.size(), GL_UNSIGNED_INT,
                       (void *) 0);

        SDL_GL_SwapWindow(window.window);
    }

} catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
