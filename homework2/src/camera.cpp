#include "camera.h"

void Camera::Move(std::map<SDL_Keycode, bool> &button_down, float dt) {
    const float cameraSpeed = 200.f * dt;   // adjust accordingly
    if (button_down[SDLK_w]) {
        cameraPos += cameraSpeed * cameraFront;
    } else if (button_down[SDLK_s]) {
        cameraPos -= cameraSpeed * cameraFront;
    } else if (button_down[SDLK_a]) {
        cameraPos -=
            glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    } else if (button_down[SDLK_d]) {
        cameraPos +=
            glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    }
    cameraFront = m.ChangeDirection();
}

Camera::Camera(Mouse &m) : m(m) {
    cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::mat4 Camera::GenView() {
    return glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}