#include "CGameEngine.h"

#include <iostream>
#include <cmath>
#include <memory>

#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "cmake_source_dir.h"
#include "shader.h"
#include "camera.h"
#include "colormap.h"
#include "filesystemmonitor.h"
#include "gui_interface.h"
#include "imgui.h"
#include "texture_utility.h"
#include "shape.h"

#include "grid.h"
#include "geomesh.h"
#include "refcamera.h"
#include "lighting.h"

#include "geocube.h"
#include "atmosphere.h"


// Shortcut
static bool bindCam = true;
static bool drawWireframe = false;
static bool drawNormalArrows = false;

Shader hdrShader;
std::unique_ptr<refCamera> refcam;
unsigned int hdrFBO;
unsigned int colorBuffer;
unsigned int rboDepth;
unsigned int hdrWidth,hdrHeight;

std::unique_ptr<Atmosphere> mesh;

void gui_interface(float h)
{
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("General::Control Panel"))
    {
        ImGui::Text("Current elevation: %f", h);
        ImGui::Checkbox("Bind camera", &bindCam);
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

    // Read shader
    //Shader shader(FP("renderer/terrain.vert"),FP("renderer/terrain.frag"));
    //Shader lodShader(FP("renderer/lod.vert"),FP("renderer/lod.frag"));
    //Shader normalShader(FP("renderer/lod.vert"),FP("renderer/lod.pcolor.frag"),FP("renderer/lod.glsl"));
    //Shader lightingShader(FP("renderer/terrain.vert"),FP("renderer/terrain.lighting.frag"));
    hdrShader.reload_shader_program_from_files(FP("renderer/hdr.vs"),FP("renderer/hdr.fs"));

    // Initlize geogrid system
    Node::init();

    // Initialize lighting system
    Lighting::init();

    // reference camera
    refCamera::shader.reload_shader_program_from_files(FP("renderer/box.vert"),FP("renderer/box.frag"));
    refcam.reset(new refCamera(GetCurrentCamera()));

    // read debug texture
    //unsigned int debug_tex = loadTexture("texture_debug.jpeg",FP("../../resources/textures"), false);
    //std::vector<std::string> faces
    //{
    //    FP("../../resources/Earth/Surface/pos_x/0_0_0_c.jpg"),
    //    FP("../../resources/Earth/Surface/neg_x/0_0_0_c.jpg"),
    //    FP("../../resources/Earth/Surface/pos_y/0_0_0_c.jpg"),
    //    FP("../../resources/Earth/Surface/neg_y/0_0_0_c.jpg"),
    //    FP("../../resources/Earth/Surface/pos_z/0_0_0_c.jpg"),
    //    FP("../../resources/Earth/Surface/neg_z/0_0_0_c.jpg")
    //};
    //uint test_img1 = loadCubemap(faces);
    uint test_img1 = loadCubemapLarge(FP("../../resources/Earth/Surface/"),"_c.jpg", 1);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, test_img1);

    //uint test_img2 = loadCubemapLarge(FP("../../resources/Earth/Surface/"),"_a.jpg", 1);
    //glActiveTexture(GL_TEXTURE11);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, test_img2);

    // gen geocube
    //Geocube mesh;
    mesh.reset(new Atmosphere(GetCurrentCamera()));
    mesh->init();

    // HDR
    // configure floating point framebuffer
    // ------------------------------------
    hdrWidth = GetFrameWidth();
    hdrHeight = GetFrameHeight();
    glGenFramebuffers(1, &hdrFBO);
    // create floating point color buffer
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, hdrWidth, hdrHeight, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // create depth buffer (renderbuffer)
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, hdrWidth, hdrHeight);
    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);

}

CGameEngine::~CGameEngine()
{
    Node::finalize();
}

void CGameEngine::Update()
{

}

void CGameEngine::RenderUpdate()
{
    if(bindCam)
    {
        //float min_height = mesh.currentElevation(refcam.Position) + camera.Near;
        //if(mesh.currentLocalHeight(camera.Position) < 5e-7)
        //    camera.Position = glm::mix(camera.Position, mesh.currentGroundPos(camera.Position, 5e-7), 0.5f);

        //camera.setReference(glm::vec3(0,1,0));
        refcam->sync_frustrum();
        refcam->sync_position();
        refcam->sync_rotation();
    }

    // Draw points
    glViewport(0,0,GetFrameWidth(), GetFrameHeight());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,hdrWidth, hdrHeight);

    // update geomesh
    mesh->getGroundHandle().update(refcam.get());

    // Draw scene
    if(drawWireframe)
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    // Draw near ground
    //glEnable(GL_DEPTH_CLAMP);
    GetCurrentCamera()->Near = 0;
    GetCurrentCamera()->Far = 1000;
    refcam->sync_frustrum();

    //mesh.drawOcean(refcam);
    mesh->drawGround(refcam.get());
    // Draw sky
    mesh->drawSky(refcam.get());

    // Restore options
    //glDisable(GL_DEPTH_CLAMP);
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Draw screen

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    hdrShader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    mesh->setHDR(hdrShader);
    renderQuad();

    // gui
    GuiInterface::Begin();
    mesh->gui_interface();
    mesh->getGroundHandle().gui_interface();
    Node::gui_interface();
    Geomesh::gui_interface();
    refcam->gui_interface();
    //dirlight.gui_interface(camera);
    gui_interface(mesh->getGroundHandle().currentGlobalHeight(refcam->Position)*6371.0);
    ImGui::ShowDemoWindow();
    GuiInterface::End();
}
