#include "camera.h"

/**
 * @brief Camera::Camera
 * @param position
 * @param up
 * @param yaw
 * @param pitch
 */
Camera::Camera(glm::vec3 position, glm::vec3 up, float zNear, float zFar, float aspectRatio, float yaw, float pitch)
    : _front(glm::vec3(0.0f, 0.0f, -1.0f))
    , _movementSpeed(SPEED)
{
    this->_zNear = zNear;
    this->_zFar = zFar;
    this->_aspectRatio = aspectRatio;
    this->_position = position;
    this->_worldUp = up;
    this->_yaw = yaw;
    this->_pitch = pitch;
    _zoom = ZOOM;
    this->updateCameraVectors();
    this->updateFrustum();
}

glm::vec3 Camera::front()
{
    return _front;
}

void Camera::aspectRatio(float aspectRatio)
{
    _aspectRatio = aspectRatio;
}

///**
// * @brief Camera::Camera
// * @param posX
// * @param posY
// * @param posZ
// * @param upX
// * @param upY
// * @param upZ
// * @param yaw
// * @param pitch
// */
// Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float zNear, float zFar, float aspectRatio, float yaw, float pitch)
//    : front(glm::vec3(0.0f, 0.0f, -1.0f))
//    , movementSpeed(SPEED)
//    , zoom(ZOOM)
//{
//    this->zNear = zNear;
//    this->zFar = zFar;
//    this->aspectRatio = aspectRatio;
//    this->position = glm::vec3(posX, posY, posZ);
//    this->worldUp = glm::vec3(upX, upY, upZ);
//    this->yaw = yaw;
//    this->pitch = pitch;
//    this->updateCameraVectors();
//}

/**
 * @brief Camera::getViewMatrix
 * @return
 */
glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(_position, _position + _front, _up);
}

/**
 * @brief Camera::getZoom
 * @return
 */
float Camera::zoom()
{
    return _zoom;
}

/**
 * @brief Camera::getPosition
 * @return
 */
glm::vec3 Camera::position()
{
    return _position;
}

/**
 * @brief Camera::processKeyboard
 * @param direction
 * @param deltaTime
 */
void Camera::processKeyboard(CameraAction direction, float deltaTime)
{
    float velocity = _movementSpeed * deltaTime;
    if (direction == CameraAction::SPEED_UP)
        _movementSpeed = SPEED * SPEED_UP_MULT;
    else
        _movementSpeed = SPEED;

    if (direction == CameraAction::MOVE_FORWARD)
        _position += _front * velocity;
    if (direction == CameraAction::MOVE_BACKWARD)
        _position -= _front * velocity;
    if (direction == CameraAction::MOVE_LEFT)
        _position -= _right * velocity;
    if (direction == CameraAction::MOVE_RIGHT)
        _position += _right * velocity;
    if (direction == CameraAction::LOOK_UP)
        _pitch = std::fmin(89, _pitch + 1);
    if (direction == CameraAction::LOOK_DOWN)
        _pitch = std::fmax(-89, _pitch - 1);
    if (direction == CameraAction::LOOK_LEFT)
        _yaw -= 1;
    if (direction == CameraAction::LOOK_RIGHT)
        _yaw += 1;

    updateCameraVectors();
}

void Camera::updateFrustum()
{
    const float halfVSide = _zFar * tanf(glm::radians(_zoom) * 0.5f);
    const float halfHSide = halfVSide * _aspectRatio;
    const glm::vec3 frontMultFar = _zFar * _front;

    viewFrustum.nearFace = { _position + _zNear * _front, _front };
    viewFrustum.farFace = { _position + frontMultFar, -_front };

    viewFrustum.rightFace = { _position,
        glm::cross(frontMultFar - _right * halfHSide, _up) };

    viewFrustum.leftFace = { _position,
        glm::cross(_up, frontMultFar + _right * halfHSide) };

    viewFrustum.topFace = { _position,
        glm::cross(_right, frontMultFar - _up * halfVSide) };

    viewFrustum.bottomFace = { _position,
        glm::cross(frontMultFar + _up * halfVSide, _right) };
}

/**
 * @brief Camera::updateCameraVectors
 */
void Camera::updateCameraVectors()
{
    glm::vec3 front1;
    front1.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    front1.y = sin(glm::radians(_pitch));
    front1.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    this->_front = glm::normalize(front1);

    _right = glm::normalize(glm::cross(_front, _worldUp));
    _up = glm::normalize(glm::cross(_right, _front));
}
