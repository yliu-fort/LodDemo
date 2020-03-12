#ifndef CAMERA_H
#define CAMERA_H

#include <GL/glew.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/vector_angle.hpp"

#include <vector>
#include <iostream>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // Camera Attributes
    glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Aspect, Near=0.1f, Far=1000.0f;
    // Euler Angles
    float Yaw = -90.0f;
    float Pitch = 0.0f;
    // Camera options
    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;
    float Zoom = 45.0f;

    // Global transformation
    glm::mat4 Rotation = glm::mat4(1);
    float wYaw, wPitch;

    // Constructor with vectors
    Camera(glm::vec3 position, float aspect, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float aspect, float yaw = -90.0f, float pitch = 0.0f);

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    const glm::mat4 GetViewMatrix() const;
    const glm::mat4 GetPerspectiveMatrix() const;
    const glm::mat4 GetPerspectiveMatrix(float,float) const;
    const glm::mat4 GetFrustumMatrix() const;
    const glm::mat4 GetFrustumMatrix(float, float) const;

    // Set properties
    void setClipping(float n=0.001f, float f=1000.0f)
    {
        Near = n, Far = f;
    }
    void setReference(glm::vec3 dir = glm::vec3(0,1,0))
    {
        dir = glm::normalize(dir);
        if(glm::length(dir - glm::vec3(0,1,0)) < 1e-6)
        {
            wYaw = 0.0f;
            wPitch = 0.0f;
            Rotation = glm::mat4(1);
        }else
        {
            // Things lost -> rolling
            glm::vec3 refPlane = glm::normalize(glm::cross(dir,WorldUp));
            wYaw = glm::orientedAngle(refPlane, glm::vec3(-1,0,0),WorldUp);
            wPitch = glm::orientedAngle(dir, WorldUp,refPlane);
            Rotation = glm::rotate(glm::rotate(glm::mat4(1),-wYaw,WorldUp), -wPitch, glm::vec3(-1,0,0));
        }
        updateCameraVectors();
    }

    void printInfo()
    {
        printf("Front: %f,%f,%f\n",Front.x,Front.y,Front.z);
        printf("Right: %f,%f,%f\n",Right.x,Right.y,Right.z);
        printf("Up: %f,%f,%f\n",Up.x,Up.y,Up.z);
        printf("Rot: %f(%f),%f(%f)\n",Pitch,glm::degrees(wPitch), Yaw ,glm::degrees(wYaw));
    }


    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);

    // Update properties when window changed
    void updateAspect(float aspect);
    void updateNearFar(float near, float far);

private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();

};

#endif
