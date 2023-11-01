#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 75.5f;
const float SPEED_UP_MULT = 4;
const float ZOOM = 45.0f;

/**
 * @brief The CameraMovement enum
 */
enum class CameraAction {
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    LOOK_UP,
    LOOK_DOWN,
    LOOK_LEFT,
    LOOK_RIGHT,
    SPEED_UP
};

/**
 * @brief Encapsulates a moving camera with various direction and position vectors.
 *
 * The basic structure of this class is based on the Camera class from
 * learnopengl.com.
 */
class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);
    glm::mat4 getViewMatrix();
    float getZoom();
    glm::vec3 getPosition();
    void processKeyboard(CameraAction direction, float deltaTime);
    // void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    float yaw;
    float pitch;
    float movementSpeed;
    float zoom;
    void updateCameraVectors();
};

#endif // CAMERA_H
