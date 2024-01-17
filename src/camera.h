#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 37.5;
const float SPEED_UP_MULT = 8;
const float ZOOM = 45.0f;
const float LOOK_VELOCITY = 2;

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
 * @brief The Plane class
 *
 * Based on learnopengl.com.
 */
struct Plane {
    glm::vec3 normal = { 0.0f, 1.0f, 0.0f };
    float distance = 0.0f;

    Plane() = default;

    Plane(const glm::vec3& p1, const glm::vec3& norm)
        : normal(glm::normalize(norm))
        , distance(glm::dot(normal, p1))
    {
    }

    float getSignedDistanceToPlane(const glm::vec3& point) const
    {
        return glm::dot(normal, point) - distance;
    }
};

/**
 * @brief The Frustum class
 *
 * Based on learnopengl.com
 */
struct Frustum {
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

/**
 * @brief Encapsulates a moving camera with various direction and position vectors.
 *
 * The basic structure of this class is based on the Camera class from
 * learnopengl.com.
 */
class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float zNear = 0.0f, float zFar = 0.0f, float aspectRatio = 1.0f, float yaw = YAW, float pitch = PITCH);
    // Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float zNear, float zFar, float aspectRatio, float yaw, float pitch);

    void processKeyboard(CameraAction direction, float deltaTime);
    void updateFrustum();
    bool insideFrustum(glm::vec3 firstPoint, glm::vec3 secondPoint);
    bool checkPlane(Plane& plane);

    /* Getters */
    glm::mat4 getViewMatrix();
    glm::vec3 front();
    glm::vec3 position();
    Frustum viewFrustum();
    float zoom();
    float yaw();
    float pitch();

    /* Setters */
    void aspectRatio(float aspectRatio);
    void yaw(float yaw);
    void pitch(float pitch);

    void intersects();

    /* Automatic flying and 360-look-around methods */
    void lerpFly(float lerpFactor);
    void lerpLook(float lerpFactor);

    bool isFlying = false;
    bool isLookingAround360 = false;

    glm::vec3 origin;
    glm::vec3 destination;
    glm::vec3 direction;
    float initialYaw;

    void updateCameraVectors();

private:
    Frustum _viewFrustum;

    glm::vec3 _position;
    glm::vec3 _front;
    glm::vec3 _up;
    glm::vec3 _right;
    glm::vec3 _worldUp;
    float _zNear;
    float _zFar;
    float _aspectRatio;
    float _yaw;
    float _pitch;
    float _movementSpeed;
    float _zoom;
    float _lookSpeed;
};

#endif // CAMERA_H
