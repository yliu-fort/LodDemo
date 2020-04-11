#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

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

    float Aspect, Near=0.001f, Far=1000.0f;
    // Euler Angles
    float Yaw = -90.0f;
    float Pitch = 0.0f;
    float Roll = 0.0f;
    // Camera options
    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;
    float Zoom = 45.0f;

    // Global transformation
    glm::quat rotation, refQuaternion;
    float wPitch=0, wYaw;

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

    glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest){
        start = normalize(start);
        dest = normalize(dest);

        float cosTheta = dot(start, dest);
        glm::vec3 rotationAxis;

        if (cosTheta < -1 + 0.001f){
            // special case when vectors in opposite directions:
            // there is no "ideal" rotation axis
            // So guess one; any will do as long as it's perpendicular to start
            rotationAxis = cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
            if (glm::length2(rotationAxis) < 0.01 ) // bad luck, they were parallel, try again!
                rotationAxis = cross(glm::vec3(1.0f, 0.0f, 0.0f), start);

            rotationAxis = normalize(rotationAxis);
            return glm::angleAxis(glm::radians(180.0f), rotationAxis);
        }

        rotationAxis = cross(start, dest);

        float s = sqrt( (1+cosTheta)*2 );
        float invs = 1 / s;

        return glm::quat(
            s * 0.5f,
            rotationAxis.x * invs,
            rotationAxis.y * invs,
            rotationAxis.z * invs
        );

    }

    void setReference(glm::vec3 dir = glm::vec3(0,1,0))
    {
        // Issue happens near south pole (0,-1,0)
        glm::quat ref = RotationBetweenVectors(glm::vec3(0,1,0), glm::normalize(dir));

        ref = glm::mix(refQuaternion, ref, 0.1f);

        wPitch += -asinf(glm::dot(ref * glm::vec3(0,1,0), refQuaternion*glm::vec3(0,0,1)));
        wYaw = acosf(glm::dot(refQuaternion * glm::vec3(0,0,1), glm::vec3(1,0,0)));
        if(glm::dot(refQuaternion * glm::vec3(0,0,1), glm::vec3(0,1,0)) > 0)
            wYaw = 2*3.141592654f - wYaw;

        //std::cout << "Pitch " << glm::degrees(wPitch) << " Yaw " << glm::degrees(wYaw) << std::endl;

        refQuaternion = ref;
        updateCameraVectors();

    }

    void printInfo()
    {
        char msg[255];
        sprintf(msg,"Front: %f,%f,%f\n",Front.x,Front.y,Front.z);
        std::cout << msg;
        sprintf(msg,"Right: %f,%f,%f\n",Right.x,Right.y,Right.z);
        std::cout << msg;
        sprintf(msg,"Up: %f,%f,%f\n",Up.x,Up.y,Up.z);
        std::cout << msg << std::endl;
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
