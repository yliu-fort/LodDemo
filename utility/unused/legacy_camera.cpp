#include "camera.h"
#include <iostream>



// Constructor with vectors
Camera::Camera(glm::vec3 position, float aspect, glm::vec3 up, float yaw, float pitch)
{
    this->Position = position;
    this->WorldUp = up;
    this->Yaw = yaw;
    this->Pitch = pitch;
    this->Aspect = aspect;
    updateCameraVectors();
}

// Constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float aspect, float yaw, float pitch)
{
    this->Position = glm::vec3(posX, posY, posZ);
    this->WorldUp = glm::vec3(upX, upY, upZ);
    this->Yaw = yaw;
    this->Pitch = pitch;
    this->Aspect = aspect;
    updateCameraVectors();
}

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
const glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

// Returns the perspective matrix calculated using Euler Angles and the LookAt Matrix
const glm::mat4 Camera::GetPerspectiveMatrix() const
{
    return glm::perspective(glm::radians(Zoom), Aspect, Near, Far);
}

const glm::mat4 Camera::GetPerspectiveMatrix(float n, float f) const
{
    return glm::perspective(glm::radians(Zoom), Aspect, n, f);
}

// Returns the assembled frustum matrix calculated using Euler Angles and the LookAt Matrix
const glm::mat4 Camera::GetFrustumMatrix() const
{
    return ((this)->GetPerspectiveMatrix()*(this)->GetViewMatrix());
}
const glm::mat4 Camera::GetFrustumMatrix(float n, float f) const
{
    return ((this)->GetPerspectiveMatrix(n,f)*(this)->GetViewMatrix());
}

// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
    updateCameraVectors();

    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;

}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;


    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        Pitch -= glm::degrees(wPitch);
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
        Pitch += glm::degrees(wPitch);
    }

    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

#define CAM_MIN_SPEED (1e-3)
#define CAM_MAX_SPEED (1e12)
// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
    /*if (Zoom >= 1.0 && Zoom <= 45.0f)
        Zoom -= yoffset;
    if (Zoom <= 1.0)
        Zoom = 1.0;
    if (Zoom >= 45.0f)
        Zoom = 45.0f;*/

    if (MovementSpeed >= CAM_MIN_SPEED && MovementSpeed <= CAM_MAX_SPEED)
        MovementSpeed *= yoffset>0?2.0f:0.5f;
    if (MovementSpeed <= CAM_MIN_SPEED)
        MovementSpeed = CAM_MIN_SPEED;
    if (MovementSpeed >= CAM_MAX_SPEED)
        MovementSpeed = CAM_MAX_SPEED;
    std::cout << "Move speed = " << MovementSpeed << std::endl;
}

// Calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors()
{
    // Calculate the new Front vector
    // front must relative to world up
    // calculate relative yaw and pitch first

    glm::vec3 front;
    front.x = cosf(glm::radians(Yaw) - wYaw) * cosf(glm::radians(Pitch) - wPitch);
    front.y = sinf(glm::radians(Pitch) - wPitch);
    front.z = sinf(glm::radians(Yaw) - wYaw) * cosf(glm::radians(Pitch) - wPitch);
    front = glm::normalize(front);
    Front = front;

    // Also re-calculate the Right and Up vector
    this->Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    this->Up    = glm::normalize(glm::cross(Right, Front));

    // Convert from local to global space
    this->Front = glm::normalize(glm::mat4x3(1)*(Rotation*glm::vec4(this->Front,1.0f)));
    this->Right = glm::normalize(glm::mat4x3(1)*(Rotation*glm::vec4(this->Right,1.0f)));
    this->Up    = glm::normalize(glm::mat4x3(1)*(Rotation*glm::vec4(this->Up   ,1.0f)));
}

void Camera::updateAspect(float a)
{
    this->Aspect = a;
}

void Camera::updateNearFar(float n, float f)
{
    this->Near = n;
    this->Far = f;
}

