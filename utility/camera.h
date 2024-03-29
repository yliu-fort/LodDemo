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
    RIGHT,
    COUNTERCLOCKWISE,
    CLOCKWISE
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

    float Aspect, Near=0.001f, Far=100.0f;
    // Euler Angles
    float Yaw = 0.0f;
    float Pitch = 0.0f;
    float Roll = 0.0f;
    // Camera options
    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.1f;
    float Zoom = glm::radians(45.0f);

    // Global transformation
    glm::quat rotation, refQuaternion;
    float wPitch=0, wYaw;

    // Constructor with vectors
    Camera(glm::vec3 position, float aspect, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float aspect, float yaw = -90.0f, float pitch = 0.0f);

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    const glm::mat4 GetViewMatrixOriginBased() const;
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


    // Horizontal mode
    void setReference(glm::vec3 dir)
    {
        dir = glm::normalize(dir);
        glm::quat ref;

        // Do not allow turnover
        if(dot(Up, dir) < 0.0f)
        {
            // turnover
            ref = angleAxis(3.1415925f, Front);
            Right = ref*Right;
        }

        //if(fabsf(dot(Right, dir)) > 1e-3f)
        glm::vec3 rAxis = glm::normalize(glm::cross(Right, dir));
        glm::vec3 desiredR = glm::normalize(glm::cross(dir, rAxis));
        ref = RotationBetweenVectors(Right, desiredR) * ref;

        rotation = ref*rotation;

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

    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();

    // gui interface
    void gui_interface(void(*forwarder)(void*) = NULL, void* object = 0);


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

    private:
};

#endif
