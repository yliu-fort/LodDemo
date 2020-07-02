#include "geocube.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

void Geocube::update(Camera* camera)
{
    self_spin();

    auto localPos = convertToLocal(camera->Position);
    top.subdivision(    localPos );
    bottom.subdivision( localPos );
    left.subdivision(   localPos );
    right.subdivision(  localPos );
    front.subdivision(  localPos );
    back.subdivision(   localPos );
}
void Geocube::draw(Shader& shader, Camera* camera)
{
    auto localPos = convertToLocal(camera->Position);
    shader.setMat4("m4ModelMatrix",this->getModelMatrix());
    top.draw(shader,    localPos );
    bottom.draw(shader, localPos );
    left.draw(shader,   localPos );
    right.draw(shader,  localPos );
    front.draw(shader,  localPos );
    back.draw(shader,   localPos );
}
float Geocube::currentElevation(const glm::vec3& pos) const
{
    auto localPos = convertToLocal(pos);
    if(top.isGroundReference(localPos))
        return top.queryElevation(localPos);
    if(bottom.isGroundReference(localPos))
        return bottom.queryElevation(localPos);
    if(left.isGroundReference(localPos))
        return left.queryElevation(localPos);
    if(right.isGroundReference(localPos))
        return right.queryElevation(localPos);
    if(front.isGroundReference(localPos))
        return front.queryElevation(localPos);
    if(back.isGroundReference(localPos))
        return back.queryElevation(localPos);
    return 0.0f;
}
float Geocube::currentLocalHeight(const glm::vec3& pos) const
{
    // h < e indicates that we are underground
    float e = currentElevation(pos);
    float h = glm::length(convertToLocal(pos))-1.0f;
    return (h-e);
}
float Geocube::currentGlobalHeight(const glm::vec3& pos) const
{
    return currentLocalHeight(pos)*scale;
}
glm::vec3 Geocube::currentGroundPos(const glm::vec3& pos, float bias) const
{
    return glm::vec3(getModelMatrix()*glm::vec4(convertToLocal(pos)*(1.0f-currentLocalHeight(pos)+bias),1.0f));
}

void Geocube::subdivision(int level)
{
    top.subdivision(    level);
    bottom.subdivision( level);
    left.subdivision(   level);
    right.subdivision(  level);
    front.subdivision(  level);
    back.subdivision(   level);
}

void Geocube::releaseAllTextureHandles()
{
    top     .releaseAllTextureHandles();
    bottom  .releaseAllTextureHandles();
    left    .releaseAllTextureHandles();
    right   .releaseAllTextureHandles();
    front   .releaseAllTextureHandles();
    back    .releaseAllTextureHandles();
}

void Geocube::self_spin()
{
    if(spinning)
        rotation.y += spin_vel*0.02f;
}

glm::mat4 Geocube::getModelMatrix() const
{
    return glm::translate(glm::scale(glm::mat4(1),glm::vec3(scale)),position)
            *glm::mat4(glm::quat(glm::radians(rotation)));
}

glm::vec3 Geocube::convertToLocal(const glm::vec3& pos) const
{
    return glm::vec3(glm::inverse(getModelMatrix())*glm::vec4(pos,1.0f));
}

#include "imgui.h"

void Geocube::gui_interface()
{
    //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Geocube::Control Panel"))
    {
        ImGui::Text("Controllable parameters for Geocube class.");

        // Transform
        ImGui::DragFloat3("Position",&(this->position)[0],0.1f);
        ImGui::DragFloat3("Rotation",&(this->rotation)[0],1.0f);
        ImGui::DragFloat("Scale",&(this->scale),0.01f,0.001f,1e6f);

        ImGui::Checkbox("Self-spin",&spinning);
        if(spinning)
        {
            ImGui::DragFloat("self-spin velocity",&spin_vel,0.001f,0.01f,0.3f);
        }

        // Global transformation
        //ImGui::DragFloat4("rotation", (float*)&rotation,0.01f);
        //ImGui::DragFloat4("refQuaternion", (float*)&refQuaternion,0.01f);

        // Info
        //ImGui::Text("Front  \t%02.6f, %02.6f, %02.6f"  , Front.x,Front.y,Front.z);
        //ImGui::Text("Up     \t%02.6f, %02.6f, %02.6f"     , Up.x,Up.y,Up.z);
        //ImGui::Text("Right  \t%02.6f, %02.6f, %02.6f"  , Right.x,Right.y,Right.z);
        //ImGui::Text("WorldUp\t%02.6f, %02.6f, %02.6f", WorldUp.x,WorldUp.y,WorldUp.z);

        ImGui::TreePop();
    }

}




