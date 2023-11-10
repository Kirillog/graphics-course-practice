#ifndef GL_H
#define GL_H

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>

#include "obj_loader.h"
#include "stb_image.h"

GLuint create_program();

void GenBuffers(obj_data &data, GLuint &model_vao, GLuint &model_vbo,
                GLuint &model_ebo);

enum class MaterialTexture { AMBIENT, ALPHA };

void LoadTextures(const obj_data &data, std::vector<GLuint> &textures,
                  MaterialTexture tex_type);
#endif