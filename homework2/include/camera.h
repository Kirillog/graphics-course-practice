#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include <SDL2/SDL.h>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include "mouse.h"
#include <map>

struct Camera {
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    Mouse &m;
    Camera(Mouse &m);

    void Move(std::map<SDL_Keycode, bool> &button_down, float dt);

    glm::mat4 GenView();
};