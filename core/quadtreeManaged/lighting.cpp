#include "lighting.h"

Shader Lighting::debugShader;
unsigned int Lighting::cubeVAO=0;
unsigned int Lighting::cubeVBO=0;

// gui interface
#include "imgui.h"

void Lighting::gui_interface(Camera& camera)
{
    if (ImGui::TreeNode("Lighting::Control Panel"))
    {
        // Draw object
        ImGui::Separator();
        ImGui::Text("Lighting.panel");

        ImGui::Text("Controllable parameters for Lighting instance.");

        // Position
        ImGui::DragFloat3("Position",&position[0],0.01f);
        ImGui::DragFloat3("Direction",&direction[0],0.01f);
        ImGui::DragFloat3("Ambient",&ambient[0],0.01f,0.0f,1.0f);
        ImGui::DragFloat3("Diffuse",&diffuse[0],0.01f,0.0f,1.0f);
        ImGui::DragFloat3("Specular",&specular[0],0.01f,0.0f,1.0f);

        // draw lighting object(debug)
        ImGui::Checkbox("Draw lighting object",&debugDrawLightingObj);
        if (debugDrawLightingObj) {this->draw(camera);}
        ImGui::TreePop();
    }


}
