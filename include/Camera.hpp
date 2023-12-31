#ifndef RICHELIEU_CAMERA_HPP
#define RICHELIEU_CAMERA_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraType {
    LookAt,
    FirstPerson,
};

class Camera {
public:
    CameraType type = CameraType::LookAt;
    float fov;
    float znear;
    float zfar;

    bool flipY = false;
    bool update = false;

    glm::vec3 position = glm::vec3();
    glm::vec3 rotation = glm::vec3();
    glm::vec4 viewPos = glm::vec4();

    glm::mat4 perspective = glm::mat4();
    glm::mat4 view = glm::mat4();

    float rotateSpeed = 1.0f;
    float moveSpeed = 1.0f;

    struct {
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
    } keys;

    bool isMoving() {
        return keys.left || keys.right || keys.up || keys.down;
    }

    void setPerspective(float znear, float zfar, float fov, float aspect) {
        this->zfar = zfar;
        this->znear = znear;
        this->fov = fov;

        perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
        if (flipY) {
            perspective[1][1] *= -1.0f;
        }
    }

    void updateAspect(float newAspect) {
        perspective = glm::perspective(glm::radians(fov), newAspect, znear, zfar);
        if (flipY) {
            perspective[1][1] *= -1.0f;
        }
    }

    void setPostion(glm::vec3 pos) {
        this->position = pos;
        updateViewMatrix();
    }

    void setRotation(glm::vec3 rot) {
        this->rotation = rot;
        updateViewMatrix();
    }


private:
    void updateViewMatrix() {
        glm::mat4 rotateMat = glm::mat4(1.0f);
        glm::mat4 transMat;

        rotateMat = glm::rotate(rotateMat, glm::radians(rotation.x * (flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
        rotateMat = glm::rotate(rotateMat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        rotateMat = glm::rotate(rotateMat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        glm::vec3 translation = position;
        if (flipY) {
            translation.y *= -1.0f;
        }
        transMat = glm::translate(glm::mat4(1.0f), translation);

        if (type == CameraType::FirstPerson)
        {
            view = rotateMat * transMat;
        }
        else
        {
            view = transMat * rotateMat;
        }

        viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

        update = true;
    }
};
#endif
