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

#include "AMRMesh.h"
#include "UCartesianMath.h"

using namespace glm;

// Shortcut
static bool bindCam = true;
static bool drawWireframe = false;
static bool drawNormalArrows = false;

Shader simpleShader, utilityShader;
Shader fieldShader;

std::unique_ptr<AMRMesh> grid;
int lodLevel = 5;
vec2 uv = vec2(0.5,0.5);

void renderAxis();
void gui_interface()
{
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("General::Control Panel"))
    {
        ImGui::SliderInt("Lod Level", &lodLevel,0,15);
        ImGui::DragFloat2("Refine Center", &(uv)[0],0.005f,0.0f,1.0f);
        ImGui::Checkbox("Draw wireframe", &drawWireframe);
        ImGui::Checkbox("Draw normal arrows", &drawNormalArrows);
        ImGui::TreePop();
    }
}

CGameEngine::CGameEngine() :
    CGameEngineAbstractBase()
{
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Read shader
    simpleShader.reload_shader_program_from_files(FP("renderer/box.vert"),FP("renderer/box.frag"));
    utilityShader.reload_shader_program_from_files(FP("renderer/axis.vert"),FP("renderer/axis.frag"));
    fieldShader.reload_shader_program_from_files(FP("renderer/field_visualizer.vert"),FP("renderer/field_visualizer.frag"));

    AMRNode::Init();

    grid.reset(new AMRMesh(mat4(1)));
    grid->Subdivision(Umath::EncodeMorton2( vec2(0.5,0.5)), (uint)5);

    GetCurrentCamera()->setClipping(0.001,100.0);


    // Test
    AutoTest();
}

CGameEngine::~CGameEngine()
{
}

void CGameEngine::Update()
{
    grid->Subdivision(Umath::EncodeMorton2( uv ), (uint)lodLevel);
    grid->MultiLevelIntegrator();
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

    fieldShader.use();
    fieldShader.setMat4("m4ViewProjectionMatrix",GetCurrentCamera()->GetFrustumMatrix());
    fieldShader.setMat4("m4ModelMatrix",rotate(mat4(1),radians(90.0f),vec3(1,0,0)));
    grid->Draw(fieldShader);

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    // Draw axis
    utilityShader.use();
    utilityShader.setMat4("m4ViewProjectionMatrix",GetCurrentCamera()->GetFrustumMatrix());
    utilityShader.setMat4("m4ModelMatrix",mat4(1));
    renderAxis();

    // gui
    GuiInterface::Begin();
    AMRMesh::GuiInterface();
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
            0.0f, 0.0f, -1e-6f,  1.0f, 0.0f, 0.0f, // to +x
            1.0f, 0.0f, -1e-6f,  1.0f, 0.0f, 0.0f, // +x
            0.0f, 0.0f, -1e-6f,  0.0f, 1.0f, 0.0f, // to +y
            0.0f, 1.0f, -1e-6f,  0.0f, 1.0f, 0.0f, // +y
            0.0f, 0.0f, -1e-6f,  0.0f, 0.0f, 1.0f, // to +z
            0.0f, 0.0f, 1.0f-1e-6f,  0.0f, 0.0f, 1.0f, // +z

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


