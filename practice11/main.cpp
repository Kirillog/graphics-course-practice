#include <cstddef>
#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string_view>
#include <vector>

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "obj_parser.hpp"
#include "stb_image.h"

std::string to_string(std::string_view str) {
  return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message) {
  throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error) {
  throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(
                                                    glewGetErrorString(error)));
}

const char vertex_shader_source[] =
    R"(#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in float in_size;
layout (location = 2) in float in_angle;

out float size;
out float angle;

void main()
{
    gl_Position = vec4(in_position, 1.0);
    size = in_size;
    angle = in_angle;
}
)";

const char geometry_shader_source[] =
    R"(#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_position;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in float size[];
in float angle[];

out vec2 texcoord;
out float out_size;

const vec2 texcoords[4] = vec2[](vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 0.0), vec2(1.0, 1.0));
const float shift_x[4] = float[](-1.0, -1.0, 1.0, 1.0);
const float shift_y[4] = float[](-1.0, 1.0, -1.0, 1.0);

void main()
{
    vec3 center = gl_in[0].gl_Position.xyz;
    vec3 Z = normalize(camera_position - center);
    vec3 X = normalize(cross(vec3(0, 1, 0), Z));
    vec3 Y = normalize(cross(Z, X));
    vec3 Xr = X * cos(angle[0]) + Y * sin(angle[0]);
    vec3 Yr = -X * sin(angle[0]) + Y * cos(angle[0]);
    for (int i = 0; i <= 4; i++) {
        gl_Position = projection * view * model * vec4(center + (Xr * shift_x[i] + Yr * shift_y[i]) * size[0], 1.0);
        texcoord = texcoords[i];
        out_size = size[0];
        EmitVertex();
    }

    EndPrimitive();
}

)";

const char fragment_shader_source[] =
    R"(#version 330 core

uniform sampler2D sampler;
uniform sampler1D sampler1;

layout (location = 0) out vec4 out_color;

in vec2 texcoord;
in float out_size;

void main()
{
    float alpha = out_size * 3 * texture(sampler, texcoord).r;
    out_color = vec4(texture(sampler1, alpha).rgb, alpha);
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

template <typename... Shaders> GLuint create_program(Shaders... shaders) {
  GLuint result = glCreateProgram();
  (glAttachShader(result, shaders), ...);
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

struct particle {
  glm::vec3 position;
  float size;
  float angle;
  glm::vec3 velocity;
  float angular_velocity;
};

GLuint load_texture(std::string const &path) {
  int width, height, channels;
  auto pixels = stbi_load(path.data(), &width, &height, &channels, 4);

  GLuint result;
  glGenTextures(1, &result);
  glBindTexture(GL_TEXTURE_2D, result);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(pixels);

  return result;
}

int main() try {
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    sdl2_fail("SDL_Init: ");

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  SDL_Window *window = SDL_CreateWindow(
      "Graphics course practice 11", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, 800, 600,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

  if (!window)
    sdl2_fail("SDL_CreateWindow: ");

  int width, height;
  SDL_GetWindowSize(window, &width, &height);

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (!gl_context)
    sdl2_fail("SDL_GL_CreateContext: ");

  if (auto result = glewInit(); result != GLEW_NO_ERROR)
    glew_fail("glewInit: ", result);

  if (!GLEW_VERSION_3_3)
    throw std::runtime_error("OpenGL 3.3 is not supported");

  glClearColor(0.f, 0.f, 0.f, 0.f);

  auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
  auto geometry_shader =
      create_shader(GL_GEOMETRY_SHADER, geometry_shader_source);
  auto fragment_shader =
      create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
  auto program =
      create_program(vertex_shader, geometry_shader, fragment_shader);

  GLuint model_location = glGetUniformLocation(program, "model");
  GLuint view_location = glGetUniformLocation(program, "view");
  GLuint projection_location = glGetUniformLocation(program, "projection");
  GLuint camera_position_location =
      glGetUniformLocation(program, "camera_position");
  GLuint sampler_location = glGetUniformLocation(program, "sampler");
  GLuint sampler1_location = glGetUniformLocation(program, "sampler1");

  std::default_random_engine rng;

  std::vector<particle> particles;

  std::function<void(particle &)> init = [&](particle &p) {
    p.position.x = std::uniform_real_distribution<float>{-0.1f, 0.1f}(rng);
    p.position.y = 0.f;
    p.position.z = std::uniform_real_distribution<float>{-0.1f, 0.1f}(rng);
    p.size = std::uniform_real_distribution<float>{0.2, 0.4}(rng);
    p.velocity.x = std::uniform_real_distribution<float>{-1.f, 1.f}(rng);
    p.velocity.y = std::uniform_real_distribution<float>{-1.f, 1.f}(rng);
    p.velocity.z = std::uniform_real_distribution<float>{-1.f, 1.f}(rng);
    p.angular_velocity = std::uniform_real_distribution<float>{-1.f, 1.f}(rng);
  };

  std::function<void(particle &, float)> update = [&](particle &p, float dt) {
    p.velocity.y += dt * 5.f;
    p.position += p.velocity * dt;
    p.velocity *= exp(-2.f * dt);
    p.size *= exp(-1.f * dt);
    p.angle += dt * p.angular_velocity;
  };

  GLuint vao, vbo;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(particle),
                        (void *)(offsetof(particle, position)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(particle),
                        (void *)(offsetof(particle, size)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(particle),
                        (void *)(offsetof(particle, angle)));

  const std::string project_root = PROJECT_ROOT;
  const std::string particle_texture_path = project_root + "/particle.png";

  GLuint texture2 = load_texture(particle_texture_path);

  GLuint texture1;

  glGenTextures(1, &texture1);
  glBindTexture(GL_TEXTURE_1D, texture1);

  uint8_t pixel_colors[] = {
      255, 0, 0, 255, 165, 0, 255, 255, 0, 255, 255, 255,
  };

  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 4, 0, GL_RGB, GL_UNSIGNED_BYTE,
               (void *)pixel_colors);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  glPointSize(5.f);

  auto last_frame_start = std::chrono::high_resolution_clock::now();

  float time = 0.f;

  std::map<SDL_Keycode, bool> button_down;

  float view_angle = 0.f;
  float camera_distance = 2.f;
  float camera_height = 0.5f;

  float camera_rotation = 0.f;

  bool paused = false;

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
          width = event.window.data1;
          height = event.window.data2;
          glViewport(0, 0, width, height);
          break;
        }
        break;
      case SDL_KEYDOWN:
        button_down[event.key.keysym.sym] = true;
        if (event.key.keysym.sym == SDLK_SPACE)
          paused = !paused;
        break;
      case SDL_KEYUP:
        button_down[event.key.keysym.sym] = false;
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

    if (button_down[SDLK_UP])
      camera_distance -= 3.f * dt;
    if (button_down[SDLK_DOWN])
      camera_distance += 3.f * dt;

    if (button_down[SDLK_LEFT])
      camera_rotation -= 3.f * dt;
    if (button_down[SDLK_RIGHT])
      camera_rotation += 3.f * dt;

    if (!paused) {
      if (particles.size() < 256) {
        particles.emplace_back(particle{});
        init(particles.back());
      }
      for (auto &particle : particles) {
        update(particle, dt);
        if (particle.position.y > 3.f || particle.size < 0.01f) {
          init(particle);
        }
      }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    float near = 0.1f;
    float far = 100.f;

    glm::mat4 model(1.f);

    glm::mat4 view(1.f);
    view = glm::translate(view, {0.f, -camera_height, -camera_distance});
    view = glm::rotate(view, view_angle, {1.f, 0.f, 0.f});
    view = glm::rotate(view, camera_rotation, {0.f, 1.f, 0.f});

    glm::mat4 projection = glm::perspective(glm::pi<float>() / 2.f,
                                            (1.f * width) / height, near, far);

    glm::vec3 camera_position =
        (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f)).xyz();

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(particle),
                 particles.data(), GL_STATIC_DRAW);

    glUseProgram(program);

    glUniformMatrix4fv(model_location, 1, GL_FALSE,
                       reinterpret_cast<float *>(&model));
    glUniformMatrix4fv(view_location, 1, GL_FALSE,
                       reinterpret_cast<float *>(&view));
    glUniformMatrix4fv(projection_location, 1, GL_FALSE,
                       reinterpret_cast<float *>(&projection));
    glUniform3fv(camera_position_location, 1,
                 reinterpret_cast<float *>(&camera_position));

    glUniform1i(sampler_location, 0);
    glUniform1i(sampler1_location, 1);

    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, texture1);
    glDrawArrays(GL_POINTS, 0, particles.size());

    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
} catch (std::exception const &e) {
  std::cerr << e.what() << std::endl;
  return EXIT_FAILURE;
}
