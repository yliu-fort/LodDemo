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
    this->rotation = glm::angleAxis(3.141592654f, WorldUp);
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
    this->rotation = glm::angleAxis(3.141592654f, WorldUp);
    updateCameraVectors();
}

const glm::mat4 Camera::GetViewMatrixOriginBased() const
{
    return glm::lookAt(glm::vec3(0), Front, Up);
}

// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
const glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

// Returns the perspective matrix calculated using Euler Angles and the LookAt Matrix
const glm::mat4 Camera::GetPerspectiveMatrix() const
{
    return glm::perspective(Zoom, Aspect, Near, Far);
}

const glm::mat4 Camera::GetPerspectiveMatrix(float n, float f) const
{
    return glm::perspective(Zoom, Aspect, n, f);
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

    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position += Right * velocity;
    if (direction == RIGHT)
        Position -= Right * velocity;
    if (direction == COUNTERCLOCKWISE)
        rotation = glm::normalize(glm::angleAxis(-deltaTime, Front)*rotation);
    if (direction == CLOCKWISE)
        rotation = glm::normalize(glm::angleAxis(deltaTime, Front)*rotation);

    updateCameraVectors();
}

// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    // Calculate rotate w.r.t global frame (not local reference frame)
    glm::quat Q1 = glm::angleAxis(0.01f*yoffset, Right);
    glm::quat Q2 = glm::angleAxis(0.01f*xoffset, Up);

    Pitch += yoffset;
    Yaw   += xoffset;

    //Pitch += yoffset*cosf(glm::radians(Roll)) + xoffset*sinf(glm::radians(Roll));
    //Yaw   += xoffset*cosf(glm::radians(Roll)) - yoffset*sinf(glm::radians(Roll));

    // if you don't do normalization, camera will be slower and slower after travel long distance
    rotation = glm::normalize((Q2*Q1)*rotation);

    // Make sure that when pitch is out of bounds, screen doesn't get flipped

    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}



// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
    /*if (Zoom >= 1.0 && Zoom <= 45.0f)
        Zoom -= yoffset;
    if (Zoom <= 1.0)
        Zoom = 1.0;
    if (Zoom >= 45.0f)
        Zoom = 45.0f;*/

    constexpr float CAM_MIN_SPEED = 1e-6f;
    constexpr float CAM_MAX_SPEED = 1e30f;

    //if (MovementSpeed >= CAM_MIN_SPEED && MovementSpeed <= CAM_MAX_SPEED)
        MovementSpeed *= yoffset>0?2.0f:0.5f;
        MovementSpeed = glm::clamp(MovementSpeed, CAM_MIN_SPEED, CAM_MAX_SPEED);
    std::cout << "Move speed = " << MovementSpeed << std::endl;
}

// Calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors()
{
    // Calculate the new Front vector
    // front must relative to world up
    // calculate relative yaw and pitch first

    //glm::vec3 EulerAngles(glm::radians(Pitch) - wPitch, glm::radians(Yaw), 0.0f);
    //glm::vec3 RollAngles(0.0f, 0.0f, glm::radians(Roll));
    // must rotate along front line
    //rotation =  glm::quat(EulerAngles);

    // Find the rotation between the front of the object (that we assume towards +Z,
    // but this depends on your model) and the desired direction
    //Front = glm::normalize(  glm::vec3(0,0,1) );

    // Recompute desiredUp so that it's perpendicular to the direction
    // You can skip that part if you really want to force desiredUp
    //Right = glm::normalize( cross(Front,  WorldUp) );
    //Up =  WorldUp;
    //glm::vec3 desiredUp = cross(Right, Front);

    // Because of the 1rst rotation, the up is probably completely screwed up.
    // Find the rotation between the "up" of the rotated object, and the desired up
    //Up = glm::normalize(rotation * glm::vec3(0,1,0) );
    //glm::quat rot2 = RotationBetweenVectors(Up, desiredUp);

    //rotation = rot2 * refQuaternion; // remember, in reverse order.

    //Right = glm::rotate(Right, glm::radians(Roll), Front);
    //Up = glm::rotate(Up, glm::radians(Roll), Front);

    //float s = cosf(glm::radians(Roll)*0.5f);
    //float invs = sinf(glm::radians(Roll)*0.5f);
    //glm::quat roll(s, Front.x * invs, Front.y * invs,Front.z * invs );

    Front =   rotation * glm::vec3(0,0,1);
    Right =   rotation * glm::vec3(1,0,0);
    Up =   rotation * glm::vec3(0,1,0);

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

#include "imgui.h"
void Camera::gui_interface(void(*forwarder)(void*), void* object)
{
    //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Camera::Control Panel"))
    {
        ImGui::Text("Controllable parameters for Camera class.");

        // Position
        ImGui::DragFloat3("Position",&Position[0],0.01f);

        ImGui::Text("Aspect %f"   ,Aspect);
        ImGui::SliderFloat("Near",&Near,1e-6f,1.0f,"%.6f",10.0f);
        ImGui::SliderFloat("Far" ,&Far ,1.0f,1e6f,"%.2f",10.0f);

        // Camera options
        ImGui::InputFloat("MovementSpeed" , &MovementSpeed,1.0f,10.0f);
        MovementSpeed = MovementSpeed > 0.0f ? MovementSpeed : 0.0f;

        //ImGui::DragFloat("MouseSensitivity") ;
        ImGui::SliderAngle(" Zoom" , &Zoom,5.0f,90.0f);

        // Global transformation
        ImGui::DragFloat4("rotation", (float*)&rotation,0.01f);
        ImGui::DragFloat4("refQuaternion", (float*)&refQuaternion,0.01f);

        // Info
        ImGui::Text("Front  \t%02.6f, %02.6f, %02.6f"  , Front.x,Front.y,Front.z);
        ImGui::Text("Up     \t%02.6f, %02.6f, %02.6f"     , Up.x,Up.y,Up.z);
        ImGui::Text("Right  \t%02.6f, %02.6f, %02.6f"  , Right.x,Right.y,Right.z);
        ImGui::Text("WorldUp\t%02.6f, %02.6f, %02.6f", WorldUp.x,WorldUp.y,WorldUp.z);

        // for derived class
        if(forwarder)
            (*forwarder)(object);

        ImGui::TreePop();
    }
}
