#include "camera.h"
#include <iostream>

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
    _zNear = zNear;
    _zFar = zFar;
    _aspectRatio = aspectRatio;
    _position = position;
    _worldUp = up;
    _yaw = yaw;
    _pitch = pitch;
    _zoom = ZOOM;

    updateCameraVectors();
    updateFrustum();
}

/**
 * @brief Camera::flyFromOrigToDest
 */
void Camera::lerpFly(float lerpFactor)
{
    _position = origin + direction * lerpFactor;
}

void Camera::lerpLook(float lerpFactor)
{
    _yaw = initialYaw + 360.0f * lerpFactor;
    updateCameraVectors();
}

glm::vec3 Camera::front()
{
    return _front;
}

void Camera::aspectRatio(float aspectRatio)
{
    _aspectRatio = aspectRatio;
}


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

void Camera::yaw(float yaw)
{
    _yaw = yaw;
}

void Camera::pitch(float pitch)
{
    _pitch = pitch;
}

float Camera::pitch()
{
    return _pitch;
}

float Camera::yaw()
{
    return _yaw;
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

Frustum Camera::viewFrustum()
{
    return _viewFrustum;
}

void Camera::updateFrustum()
{
    const float halfVSide = _zFar * tanf(glm::radians(_zoom) * 0.5f);
    const float halfHSide = halfVSide * _aspectRatio;
    const glm::vec3 frontMultFar = _zFar * _front;

    _viewFrustum.nearFace = { _position + _zNear * _front, _front };
    _viewFrustum.farFace = { _position + frontMultFar, -_front };

    _viewFrustum.rightFace = { _position,
        glm::cross(frontMultFar - _right * halfHSide, _up) };

    _viewFrustum.leftFace = { _position,
        glm::cross(_up, frontMultFar + _right * halfHSide) };

    _viewFrustum.topFace = { _position,
        glm::cross(_right, frontMultFar - _up * halfVSide) };

    _viewFrustum.bottomFace = { _position,
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
    _front = glm::normalize(front1);

    _right = glm::normalize(glm::cross(_front, _worldUp));
    _up = glm::normalize(glm::cross(_right, _front));
}
