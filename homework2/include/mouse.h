#pragma once

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec3.hpp>

struct Mouse {
    bool firstMouse = true;
    float xpos, ypos;
    float lastX, lastY;

    float yaw = 0, pitch = 0;

    glm::vec3 ChangeDirection();
};