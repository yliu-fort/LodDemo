#include "CGameEngine.h"

#include <iostream>
#include <cmath>
#include <memory>

#include <glad/glad.h>

#include "cmake_source_dir.h"
#include "shader.h"
#include "camera.h"
#include "colormap.h"
#include "filesystemmonitor.h"
#include "gui_interface.h"
#include "imgui.h"
#include "texture_utility.h"
#include "shape.h"

#include "OGeomesh.h"

// Shortcut
static bool bindCam = true;
static bool drawWireframe = false;
static bool drawNormalArrows = false;

Shader simpleShader, curveShader, utilityShader;

glm::mat4 m4ModelMatrix = glm::mat4(1);
std::unique_ptr<OGeomesh> grid;
float radius = 1.0f;

void renderAxis();
void gui_interface()
{
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("General::Control Panel"))
    {
        ImGui::DragFloat("Radius", &radius,1.0);
        ImGui::Checkbox("Draw wireframe", &drawWireframe);
        ImGui::Checkbox("Draw normal arrows", &drawNormalArrows);
        ImGui::TreePop();
    }
}

CGameEngine::CGameEngine() :
    CGameEngineAbstractBase()
{
    // Test
    AutoTest();

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // Read shader
    simpleShader.reload_shader_program_from_files(FP("renderer/box.vert"),FP("renderer/box.frag"));
    curveShader.reload_shader_program_from_files(FP("renderer/curved.vert"),FP("renderer/curved.frag"));
    utilityShader.reload_shader_program_from_files(FP("renderer/axis.vert"),FP("renderer/axis.frag"));
    grid.reset(new OGeomesh);

}

CGameEngine::~CGameEngine()
{
}

void CGameEngine::Update()
{

}

void CGameEngine::RenderUpdate()
{

    // Resize frame
    glViewport(0,0,GetFrameWidth(), GetFrameHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw scene
    if(drawWireframe)
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    //simpleShader.use();
    //simpleShader.setMat4("m4ViewProjectionMatrix",GetCurrentCamera()->GetFrustumMatrix());
    //simpleShader.setMat4("m4ModelMatrix",glm::mat4(1));
    //grid->Draw(simpleShader,GetCurrentCamera()->Position);

    curveShader.use();
    curveShader.setMat4("m4ViewProjectionMatrix",GetCurrentCamera()->GetFrustumMatrix());
    curveShader.setMat4("m4ModelMatrix",glm::mat4(1));
    curveShader.setFloat("phi",1.0/radius);
    curveShader.setFloat("theta",1.0/radius);
    curveShader.setFloat("radius",radius);
    grid->Draw(curveShader,GetCurrentCamera()->Position);


    utilityShader.use();
    utilityShader.setMat4("m4ViewProjectionMatrix",GetCurrentCamera()->GetFrustumMatrix());
    utilityShader.setMat4("m4ModelMatrix",glm::mat4(1));
    renderAxis();

    // gui
    GuiInterface::Begin();
    gui_interface();
    ImGui::ShowDemoWindow();
    GuiInterface::End();
}

static unsigned int axisVAO = 0;
static unsigned int axisVBO = 0;

void renderAxis()
{
    // initialize (if necessary)
    if (axisVAO == 0)
    {
        float vertices[] = {
            // z dir
            0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, // to +x
            1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f, // +x
            0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f, // to +y
            0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f, // +y
            0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f, // to +z
            0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f, // +z

        };
        glGenVertexArrays(1, &axisVAO);
        glGenBuffers(1, &axisVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(axisVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(axisVAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
}


