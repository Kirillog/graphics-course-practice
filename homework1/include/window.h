#ifndef WINDOW_H
#define WINDOW_H
#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>
#include <string>
#include <string_view>
#include <stdexcept>


class Window {
public:
    Window();

    ~Window();
private:
    SDL_GLContext context;
public:
    SDL_Window * window;
    int width, height;
};


#endif