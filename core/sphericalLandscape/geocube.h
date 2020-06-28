#pragma once

#include "geomesh.h"

#include "glm/glm.hpp"


#include "shader.h"
#include "camera.h"

class Geocube
{
    Geomesh top, bottom, left, right, front, back;

    glm::vec3 position,rotation;
    float scale;
    bool spinning = false;
    float spin_vel = 0.1f;

public:
    Geocube():position(0),rotation(0),scale(1)
        ,top(Geomesh(glm::translate(glm::mat4(1),glm::vec3(0,1,0))))
        ,bottom(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(180.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))))
        ,left(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(90.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))))
        ,right(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(-90.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))))
        ,front(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(90.0f),glm::vec3(1,0,0)),glm::vec3(0,1,0))))
        ,back(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(-90.0f),glm::vec3(1,0,0)),glm::vec3(0,1,0))))
    {}
    void update(Camera& camera);
    void draw(Shader& shader, Camera& camera);
    float currentElevation(const glm::vec3& pos) const;
    float currentLocalHeight(const glm::vec3& pos) const;
    float currentGlobalHeight(const glm::vec3& pos) const;
    glm::vec3 currentGroundPos(const glm::vec3& pos, float bias) const;
    void self_spin();
    void gui_interface();

    void setPosition(glm::vec3 p)
    {
        position = p;
    }
    void setRotation(glm::vec3 r)
    {
        rotation = r;
    }
    void setScale(float t)
    {
        scale = t;
    }
protected:
    glm::mat4 getModelMatrix() const;
    glm::vec3 convertToLocal(const glm::vec3& pos) const;

};
