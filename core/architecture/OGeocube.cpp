#include "OGeocube.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

void OGeocube::Update(Camera* camera)
{
    SelfSpin();

    auto localPos = ConvertToLocal(camera->Position);
    top     ->Subdivision( localPos );
    bottom  ->Subdivision( localPos );
    left    ->Subdivision( localPos );
    right   ->Subdivision( localPos );
    front   ->Subdivision( localPos );
    back    ->Subdivision( localPos );
}
void OGeocube::Draw(Shader& shader, Camera* camera)
{
    auto localPos = ConvertToLocal(camera->Position);
    shader.setMat4("m4ModelMatrix",this->GetModelMatrix());
    top     ->Draw(shader, localPos );
    bottom  ->Draw(shader, localPos );
    left    ->Draw(shader, localPos );
    right   ->Draw(shader, localPos );
    front   ->Draw(shader, localPos );
    back    ->Draw(shader, localPos );
}
float OGeocube::CurrentElevation(const glm::vec3& pos) const
{
    auto localPos = ConvertToLocal(pos);
    if(top->IsGroundReference(localPos))
        return top->QueryElevation(localPos);
    if(bottom->IsGroundReference(localPos))
        return bottom->QueryElevation(localPos);
    if(left->IsGroundReference(localPos))
        return left->QueryElevation(localPos);
    if(right->IsGroundReference(localPos))
        return right->QueryElevation(localPos);
    if(front->IsGroundReference(localPos))
        return front->QueryElevation(localPos);
    if(back->IsGroundReference(localPos))
        return back->QueryElevation(localPos);
    return 0.0f;
}
float OGeocube::CurrentLocalHeight(const glm::vec3& pos) const
{
    // h < e indicates that we are underground
    float e = CurrentElevation(pos);
    float h = glm::length(ConvertToLocal(pos))-1.0f;
    return (h-e);
}
float OGeocube::CurrentGlobalHeight(const glm::vec3& pos) const
{
    return CurrentLocalHeight(pos)*scale;
}
glm::vec3 OGeocube::CurrentGroundPos(const glm::vec3& pos, float bias) const
{
    return glm::vec3(GetModelMatrix()*glm::vec4(ConvertToLocal(pos)*(1.0f-CurrentLocalHeight(pos)+bias),1.0f));
}

void OGeocube::Subdivision(int level)
{
    top     ->Subdivision(    level);
    bottom  ->Subdivision( level);
    left    ->Subdivision(   level);
    right   ->Subdivision(  level);
    front   ->Subdivision(  level);
    back    ->Subdivision(   level);
}

void OGeocube::ReleaseAllTextureHandles()
{
    top     ->ReleaseAllTextureHandles();
    bottom  ->ReleaseAllTextureHandles();
    left    ->ReleaseAllTextureHandles();
    right   ->ReleaseAllTextureHandles();
    front   ->ReleaseAllTextureHandles();
    back    ->ReleaseAllTextureHandles();
}

void OGeocube::SelfSpin()
{
    if(spinning)
        rotation.y += spin_vel*0.02f;
}

glm::mat4 OGeocube::GetModelMatrix() const
{
    return glm::translate(glm::scale(glm::mat4(1),glm::vec3(scale)),position)
            *glm::mat4(glm::quat(glm::radians(rotation)));
}

glm::vec3 OGeocube::ConvertToLocal(const glm::vec3& pos) const
{
    return glm::vec3(glm::inverse(GetModelMatrix())*glm::vec4(pos,1.0f));
}

#include "imgui.h"

void OGeocube::GuiInterface()
{
    //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("OGeocube::Control Panel"))
    {
        ImGui::Text("Controllable parameters for OGeocube class.");

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
        //ImGui::Text("Front  \t%02.6f, %02.6f, %02.6f"  , front->x,front->y,front->z);
        //ImGui::Text("Up     \t%02.6f, %02.6f, %02.6f"     , Up.x,Up.y,Up.z);
        //ImGui::Text("Right  \t%02.6f, %02.6f, %02.6f"  , right->x,right->y,right->z);
        //ImGui::Text("WorldUp\t%02.6f, %02.6f, %02.6f", WorldUp.x,WorldUp.y,WorldUp.z);

        ImGui::TreePop();
    }

}




