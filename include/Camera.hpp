#ifndef RICHELIEU_CAMERA_HPP
#define RICHELIEU_CAMERA_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::vec3 position = glm::vec3();
    glm::vec3 rotation = glm::vec3();
    glm::vec4 viewPos = glm::vec4();

    void setPostion(glm::vec3 pos) {
        this->position = pos;
        updateViewMatrix();
    }

    void setRotation(glm::vec3 rot) {
        this->rotation = rot;
        updateViewMatrix();
    }


private:
    float fov;

    void updateViewMatrix() {

    }
};
#endif
