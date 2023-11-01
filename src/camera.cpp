#include "camera.h"

/**
 * @brief Camera::Camera
 * @param position
 * @param up
 * @param yaw
 * @param pitch
 */
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f))
    , movementSpeed(SPEED)
    , zoom(ZOOM)
{
    this->position = position;
    this->worldUp = up;
    this->yaw = yaw;
    this->pitch = pitch;
    this->updateCameraVectors();
}

/**
 * @brief Camera::Camera
 * @param posX
 * @param posY
 * @param posZ
 * @param upX
 * @param upY
 * @param upZ
 * @param yaw
 * @param pitch
 */
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : front(glm::vec3(0.0f, 0.0f, -1.0f))
    , movementSpeed(SPEED)
    , zoom(ZOOM)
{
    this->position = glm::vec3(posX, posY, posZ);
    this->worldUp = glm::vec3(upX, upY, upZ);
    this->yaw = yaw;
    this->pitch = pitch;
    this->updateCameraVectors();
}

/**
 * @brief Camera::getViewMatrix
 * @return
 */
glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(position, position + front, up);
}

/**
 * @brief Camera::getZoom
 * @return
 */
float Camera::getZoom()
{
    return zoom;
}

/**
 * @brief Camera::getPosition
 * @return
 */
glm::vec3 Camera::getPosition()
{
    return position;
}

/**
 * @brief Camera::processKeyboard
 * @param direction
 * @param deltaTime
 */
void Camera::processKeyboard(CameraAction direction, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;
    if (direction == CameraAction::SPEED_UP)
        movementSpeed = SPEED * SPEED_UP_MULT;
    else
        movementSpeed = SPEED;

    if (direction == CameraAction::MOVE_FORWARD)
        position += front * velocity;
    if (direction == CameraAction::MOVE_BACKWARD)
        position -= front * velocity;
    if (direction == CameraAction::MOVE_LEFT)
        position -= right * velocity;
    if (direction == CameraAction::MOVE_RIGHT)
        position += right * velocity;
    if (direction == CameraAction::LOOK_UP)
        pitch = std::fmin(89, pitch + 1);
    if (direction == CameraAction::LOOK_DOWN)
        pitch = std::fmax(-89, pitch - 1);
    if (direction == CameraAction::LOOK_LEFT)
        yaw -= 1;
    if (direction == CameraAction::LOOK_RIGHT)
        yaw += 1;

    updateCameraVectors();
}

/**
 * @brief Camera::updateCameraVectors
 */
void Camera::updateCameraVectors()
{
    glm::vec3 front1;
    front1.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front1.y = sin(glm::radians(pitch));
    front1.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    this->front = glm::normalize(front1);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
