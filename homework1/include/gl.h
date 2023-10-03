#ifndef GL_H
#define GL_H
#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>
#include <string>
#include <stdexcept>

GLuint create_program();


#endif