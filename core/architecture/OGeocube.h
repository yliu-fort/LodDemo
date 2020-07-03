#pragma once

#include "OGeomesh.h"

#include "glm/glm.hpp"

#include <memory>
#include "shader.h"
#include "camera.h"

class OGeocube
{
    std::unique_ptr<OGeomesh> top, bottom, left, right, front, back;

    glm::vec3 position,rotation;
    float scale;
    bool spinning = false;
    float spin_vel = 0.1f;

public:
    OGeocube():position(0),rotation(0),scale(1)
    {
        top.reset(new OGeomesh(glm::translate(glm::mat4(1),glm::vec3(0,1,0))));
        bottom.reset(new OGeomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(180.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))));
        left.reset(new OGeomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(90.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))));
        right.reset(new OGeomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(-90.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))));
        front.reset(new OGeomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(90.0f),glm::vec3(1,0,0)),glm::vec3(0,1,0))));
        back.reset(new OGeomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(-90.0f),glm::vec3(1,0,0)),glm::vec3(0,1,0))));
    }
    void Update(Camera* camera);
    void Draw(Shader& shader, Camera* camera);
    void Subdivision(int);
    void ReleaseAllTextureHandles();
    float CurrentElevation(const glm::vec3& pos) const;
    float CurrentLocalHeight(const glm::vec3& pos) const;
    float CurrentGlobalHeight(const glm::vec3& pos) const;
    glm::vec3 CurrentGroundPos(const glm::vec3& pos, float bias) const;
    void SelfSpin();
    void GuiInterface();

    void SetPosition(glm::vec3 p)
    {
        position = p;
    }
    void SetRotation(glm::vec3 r)
    {
        rotation = r;
    }
    void SetScale(float t)
    {
        scale = t;
    }
protected:
    glm::mat4 GetModelMatrix() const;
    glm::vec3 ConvertToLocal(const glm::vec3& pos) const;

};
