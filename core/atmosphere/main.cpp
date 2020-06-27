#include <iostream>
#include <cmath>

#include <glad/glad.h>

//GLFW
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "cmake_source_dir.h"
#include "shader.h"
#include "camera.h"
#include "filesystemmonitor.h"
#include "texture_utility.h"
#include "icosphere.h"

#include "gui_interface.h"
#include "imgui.h"
//#include "Icosphere.h"

#define PI (3.141592654)

// settings
static int SCR_WIDTH  = 1600;
static int SCR_HEIGHT = 900;

// camera
static Camera m_3DCamera = Camera(glm::vec3(0.0f, 0.0f, 2.50f), float(SCR_WIDTH)/SCR_HEIGHT);
static auto m_vLight = glm::vec3(0, 0, 1000);
static auto m_vLightDirection = glm::normalize(m_vLight);
static auto m_vRotation = glm::vec3(0, 0, 0);

static auto m_nSamples = 3;		// Number of sample rays to use in integral equation
static auto m_Kr = 0.0025f;		// Rayleigh scattering constant
static auto m_Kr4PI = m_Kr*4.0f*PI;
static auto m_Km = 0.0010f;		// Mie scattering constant
static auto m_Km4PI = m_Km*4.0f*PI;
static auto m_ESun = 20.0f;		// Sun brightness constant
////For Mie aerosol scattering, g is usually set between -0.75 and -0.999
static auto m_g = -0.990f;		// The Mie phase asymmetry factor
static auto m_fExposure = 1.0f;

static auto m_fInnerRadius = 1.0f;
static auto m_fOuterRadius = 1.0126f;
static auto m_fScale = 1 / (m_fOuterRadius - m_fInnerRadius);

static float m_fWavelength[3]{
    0.650f,     // 650 nm for red
    0.570f,     // 570 nm for green
    0.475f      // 475 nm for blue
};

static float m_fWavelength4[3] {
    powf(m_fWavelength[0], 4.0f),
            powf(m_fWavelength[1], 4.0f),
            powf(m_fWavelength[2], 4.0f)
};

////My implementation uses 0.25, so the average density is found 25 percent of the way up from the ground to the sky dome.
static auto m_fRayleighScaleDepth = 0.1f;
static auto m_fMieScaleDepth = 0.1f;
static bool m_fHdr = true;

static unsigned int m_tOpticalDepthBuffer, m_tPhaseBuffer;
static int m_nODBSize = 64;
static int m_nODBSamples = 50;

static float lastX = SCR_WIDTH / 2.0f;
static float lastY = SCR_HEIGHT / 2.0f;
static bool firstMouse = true;

// timing
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

// fps recording
static float lastFpsCountFrame = 0;
static int frameCount = 0;

// Pre-declaration
GLFWwindow* initGL(int w, int h);
bool countAndDisplayFps(GLFWwindow* window);
void processInput(GLFWwindow *window);

void renderBox();
void renderQuad();

static void MakeOpticalDepthBuffer(float fInnerRadius, float fOuterRadius, float fRayleighScaleHeight, float fMieScaleHeight)
{
    const float DELTA = 1e-6;
    const int m_nChannels = 4;
    const int nSize = m_nODBSize;
    const int nSamples = m_nODBSamples;
    const float fScale = 1.0f / (fOuterRadius - fInnerRadius);


    std::vector<float> m_pBuffer(m_nChannels*nSize*nSize);
    //Init(nSize, nSize, 1, 4, GL_RGBA, GL_FLOAT);
    int nIndex = 0;
    float fPrev = 0;
    for(int nAngle=0; nAngle<nSize; nAngle++)
    {
        // As the y tex coord goes from 0 to 1, the angle goes from 0 to 180 degrees
        float fCos = 1.0f - (nAngle+nAngle) / (float)nSize;
        float fAngle = acosf(fCos);
        glm::vec3 vRay(sinf(fAngle), cosf(fAngle), 0);	// Ray pointing to the viewpoint

        float fFirst = 0;
        for(int nHeight=0; nHeight<nSize; nHeight++)
        {
            // As the x tex coord goes from 0 to 1, the height goes from the bottom of the atmosphere to the top
            float fHeight = DELTA + fInnerRadius + ((fOuterRadius - fInnerRadius) * nHeight) / nSize;
            glm::vec3 vPos(0, fHeight, 0);				// The position of the camera

            // If the ray from vPos heading in the vRay direction intersects the inner radius (i.e. the planet), then this spot is not visible from the viewpoint
            float B = 2.0f * glm::dot(vPos, vRay);
            float Bsq = B * B;
            float Cpart = glm::dot(vPos, vPos);
            float C = Cpart - fInnerRadius*fInnerRadius;
            float fDet = Bsq - 4.0f * C;
            bool bVisible = (fDet < 0 || ((0.5f * (-B - sqrtf(fDet)) <= 0) && (0.5f * (-B + sqrtf(fDet)) <= 0)));
            float fRayleighDensityRatio;
            float fMieDensityRatio;
            if(bVisible)
            {
                fRayleighDensityRatio = expf(-(fHeight - fInnerRadius) * fScale / fRayleighScaleHeight);
                fMieDensityRatio = expf(-(fHeight - fInnerRadius) * fScale / fMieScaleHeight);
            }
            else
            {
                // Smooth the transition from light to shadow (it is a soft shadow after all)
                fRayleighDensityRatio = (m_pBuffer)[nIndex - nSize*m_nChannels] * 0.5f;
                fMieDensityRatio = (m_pBuffer)[nIndex+2 - nSize*m_nChannels] * 0.5f;
            }

            // Determine where the ray intersects the outer radius (the top of the atmosphere)
            // This is the end of our ray for determining the optical depth (vPos is the start)
            C = Cpart - fOuterRadius*fOuterRadius;
            fDet = Bsq - 4.0f * C;
            float fFar = 0.5f * (-B + sqrtf(fDet));

            // Next determine the length of each sample, scale the sample ray, and make sure position checks are at the center of a sample ray
            float fSampleLength = fFar / nSamples;
            float fScaledLength = fSampleLength * fScale;
            glm::vec3 vSampleRay = vRay * fSampleLength;
            vPos += vSampleRay * 0.5f;

            // Iterate through the samples to sum up the optical depth for the distance the ray travels through the atmosphere
            float fRayleighDepth = 0;
            float fMieDepth = 0;
            for(int i=0; i<nSamples; i++)
            {
                float fHeight = glm::length(vPos);
                float fAltitude = (fHeight - fInnerRadius) * fScale;
                //fAltitude = fmaxf(fAltitude, 0.0f);
                fRayleighDepth += expf(-fAltitude / fRayleighScaleHeight);
                fMieDepth += expf(-fAltitude / fMieScaleHeight);
                vPos += vSampleRay;
            }

            // Multiply the sums by the length the ray traveled
            fRayleighDepth *= fScaledLength;
            fMieDepth *= fScaledLength;

            // happens on the back side
            //if(!_finite(fRayleighDepth) || fRayleighDepth > 1.0e25f)
            //    fRayleighDepth = 0;
            //if(!_finite(fMieDepth) || fMieDepth > 1.0e25f)
            //    fMieDepth = 0;

            // Store the results for Rayleigh to the light source, Rayleigh to the camera, Mie to the light source, and Mie to the camera
            (m_pBuffer)[nIndex++] = fRayleighDensityRatio;
            (m_pBuffer)[nIndex++] = fRayleighDepth;
            (m_pBuffer)[nIndex++] = fMieDensityRatio;
            (m_pBuffer)[nIndex++] = fMieDepth;

        }
        //ofGraph << std::endl;
    }

    if(m_tOpticalDepthBuffer)
        glDeleteTextures(1,&m_tOpticalDepthBuffer);
    glGenTextures(1, &m_tOpticalDepthBuffer);
    glBindTexture(GL_TEXTURE_2D, m_tOpticalDepthBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, nSize, nSize, 0, GL_RGBA, GL_FLOAT, &m_pBuffer[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

}

static void MakePhaseBuffer(float ESun, float Kr, float Km, float g)
{
    const int m_nWidth = 64;
    std::vector<float> m_pBuffer(2*m_nWidth);

    auto KmSun = Km*ESun;
    auto KrSun = Kr*ESun;
    float g2 = g*g;
    float fMiePart = 1.5f * (1.0f - g2) / (2.0f + g2);

    int nIndex = 0;
    for(int nAngle=0; nAngle<m_nWidth; nAngle++)
    {
        float fCos = 1.0f - (nAngle+nAngle) / (float)m_nWidth;
        float fCos2 = fCos*fCos;
        float fRayleighPhase = 0.75f * (1.0f + fCos2);
        float fMiePhase = fMiePart * (1.0f + fCos2) / powf(1.0f + g2 - 2.0f*g*fCos, 1.5f);
        (m_pBuffer)[nIndex++] = fRayleighPhase * KrSun;
        (m_pBuffer)[nIndex++] = fMiePhase * KmSun;
    }

    if(m_tPhaseBuffer)
        glDeleteTextures(1,&m_tPhaseBuffer);
    glGenTextures(1, &m_tPhaseBuffer);
    glBindTexture(GL_TEXTURE_1D, m_tPhaseBuffer);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RG32F, m_nWidth, 0, GL_RG, GL_FLOAT, &m_pBuffer[0]);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

}

static void update()
{
    m_fWavelength4[0] = powf(m_fWavelength[0], 4.0f);
    m_fWavelength4[1] = powf(m_fWavelength[1], 4.0f);
    m_fWavelength4[2] = powf(m_fWavelength[2], 4.0f);


    m_Kr4PI = m_Kr*4.0f*PI;
    m_Km4PI = m_Km*4.0f*PI;
    m_vLightDirection = glm::normalize(m_vLight);
    m_fScale = 1 / (m_fOuterRadius - m_fInnerRadius);

    MakeOpticalDepthBuffer(m_fInnerRadius,m_fOuterRadius,m_fRayleighScaleDepth,m_fMieScaleDepth);
    MakePhaseBuffer(m_ESun, m_Kr, m_Km, m_g);
}

static void reset()
{
    m_vLight = glm::vec3(0, 0, 1000);

    m_nSamples = 3;		// Number of sample rays to use in integral equation
    m_Kr = 0.0025f;		// Rayleigh scattering constant
    m_Km = 0.0010f;		// Mie scattering constant
    m_ESun = 20.0f;		// Sun brightness constant
    m_g = -0.990f;		// The Mie phase asymmetry factor
    m_fExposure = 1.0f;

    m_fInnerRadius = 1.0f;
    m_fOuterRadius = 1.0126f;

    m_fWavelength[0] = 0.650f;
    m_fWavelength[1] = 0.570f;
    m_fWavelength[2] = 0.475f;

    m_fRayleighScaleDepth = 0.1f;
    m_fMieScaleDepth = 0.1f;
    m_fHdr = true;

    update();
}

static void gui_interface()
{
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Atmosphere::Control Panel"))
    {
        ImGui::Text("Controllable parameters for Atmosphere class.");

        // Transform
        ImGui::DragFloat3("Planet Rotation",&(m_vRotation)[0]);
        ImGui::DragFloat3("Light Position",&(m_vLight)[0]);
        m_vLightDirection = glm::normalize(m_vLight);
        ImGui::DragFloat3("Wave length",&(m_fWavelength)[0],0.001);
        ImGui::DragInt("m_nSamples",&m_nSamples,1,1,16);
        ImGui::DragInt("ODBsize",&m_nODBSize,16,16,1024);
        ImGui::DragInt("ODBsamples",&m_nODBSamples,1,16,256);

        ImGui::DragFloat("Rayleigh", &m_Kr,0.0001);		// Rayleigh scattering constant
        ImGui::DragFloat("Mie", &m_Km,0.0001);		// Mie scattering constant
        ImGui::DragFloat("Brightness", &m_ESun,0.1);		// Sun brightness constant
        ImGui::DragFloat("Mie phase asymmetry", &m_g,0.001,-0.999,-0.75);		// The Mie phase asymmetry factor

        ImGui::DragFloat("Inner Radius", &m_fInnerRadius,0.1,0.1,m_fOuterRadius);
        ImGui::DragFloat("Outer Radius", &m_fOuterRadius,0.1,m_fInnerRadius,9999);

        ImGui::DragFloat("Rayleigh ScaleDepth", &m_fRayleighScaleDepth,0.01);
        ImGui::DragFloat("Mie ScaleDepth", &m_fMieScaleDepth,0.01);

        ImGui::Checkbox("HDR",&m_fHdr);
        if(m_fHdr)
        {
            ImGui::DragFloat("HDR Exposure", &m_fExposure,0.01 );
        }

        if(ImGui::Button("Reset"))
            reset();

        if(ImGui::Button("Update Buffers"))
            update();

        // Global transformation
        //ImGui::DragFloat4("rotation", (float*)&rotation,0.01f);
        //ImGui::DragFloat4("refQuaternion", (float*)&refQuaternion,0.01f);

        // Info
        if(glm::length(m_3DCamera.Position) >= m_fOuterRadius)
            ImGui::Text("In vaccum.");
        else
            ImGui::Text("In atmosphere.");
        //ImGui::Text("Front  \t%02.6f, %02.6f, %02.6f"  , Front.x,Front.y,Front.z);
        //ImGui::Text("Up     \t%02.6f, %02.6f, %02.6f"     , Up.x,Up.y,Up.z);
        //ImGui::Text("Right  \t%02.6f, %02.6f, %02.6f"  , Right.x,Right.y,Right.z);
        //ImGui::Text("WorldUp\t%02.6f, %02.6f, %02.6f", WorldUp.x,WorldUp.y,WorldUp.z);

        ImGui::TreePop();
    }

}

int main()
{
#if defined(__linux__)
    setenv ("DISPLAY", ":0", 0);
#endif

    // Initialize a window
    GLFWwindow* window = initGL(SCR_WIDTH, SCR_HEIGHT);
    printf("Initial glwindow...\n");
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);

    // Read shader
    //Shader shader(FP("renderer/icosphere.vert"),FP("renderer/icosphere.frag"));
    Shader lightShader(FP("renderer/icosphere.vert"),FP("renderer/icosphere_with_light.frag"));
    //Shader lineShader(FP("renderer/icosphere.vert"),FP("renderer/icosphere_line.frag"));
    Shader hdrShader(FP("renderer/hdr.vs"),FP("renderer/hdr.fs"));

    Shader m_shSkyFromSpace         (FP("atmosphereRender/SkyFromSpace.vert"          ),FP("atmosphereRender/SkyFromSpace.frag"         ));
    Shader m_shSkyFromAtmosphere    (FP("atmosphereRender/SkyFromAtmosphere.vert"     ),FP("atmosphereRender/SkyFromAtmosphere.frag"    ));
    Shader m_shGroundFromSpace      (FP("atmosphereRender/GroundFromSpace.vert"       ),FP("atmosphereRender/GroundFromSpace.frag"      ));
    Shader m_shGroundFromAtmosphere (FP("atmosphereRender/GroundFromAtmosphere.vert"  ),FP("atmosphereRender/GroundFromAtmosphere.frag" ));
    Shader m_shSpaceFromSpace       (FP("atmosphereRender/SpaceFromSpace.vert"        ),FP("atmosphereRender/SpaceFromSpace.frag"       ));
    Shader m_shSpaceFromAtmosphere  (FP("atmosphereRender/SpaceFromAtmosphere.vert"   ),FP("atmosphereRender/SpaceFromAtmosphere.frag"  ));

    // For automatic file reloading
    //FileSystemMonitor::Init(SRC_PATH);
    GuiInterface::Init(window);

    // Prepare buffers
    Icosphere m_tEarth(6);
    //Icosphere m_tMoon(3);

    // load textures
    // -------------
    uint texture1 = loadTexture("5k_earth_daymap_bathy.jpg", FP("../../resources/textures/earth"), true);
    //uint texture12 = loadTexture("2k_earth_clouds.jpg", FP("../../resources/textures/earth"));
    uint texture2 = loadTexture("2k_earth_specular_map.jpg", FP("../../resources/textures/earth"));
    uint texture3 = loadTexture("256_earth_bumpmap.jpg", FP("../../resources/textures/earth"));


    // configure floating point framebuffer
    // ------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    // create floating point color buffer
    unsigned int colorBuffer;
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // create depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);

    // Pre-compute atmosphere
    update();


    while( !glfwWindowShouldClose( window ) )
    {
        // per-frame time logic
        // --------------------
        countAndDisplayFps(window);


        // input
        glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
        processInput(window);

        // Draw points
        glViewport(0,0,SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bound camera position
        if(glm::length(m_3DCamera.Position) < m_fInnerRadius + m_3DCamera.Near)
            m_3DCamera.Position = glm::normalize(m_3DCamera.Position) * (m_fInnerRadius + m_3DCamera.Near);

        // get reference
        auto& vCamera = m_3DCamera.Position;
        Shader *pGroundShader, *pSkyShader, *pSpaceShader;

        if(glm::length(vCamera) >= m_fOuterRadius)
        {
            pSpaceShader = &m_shSpaceFromSpace;
            pGroundShader = &m_shGroundFromSpace;
            pSkyShader = &m_shSkyFromSpace;
        }
        else
        {
            pSpaceShader = &m_shSpaceFromAtmosphere;
            pGroundShader = &m_shGroundFromAtmosphere;
            pSkyShader = &m_shSkyFromAtmosphere;
        }

        {
            // Draw ground
            pGroundShader->use();
            pGroundShader->setVec3("v3CameraPos", vCamera.x, vCamera.y, vCamera.z);
            pGroundShader->setVec3("v3LightDir", m_vLightDirection);
            pGroundShader->setVec3("v3InvWavelength", 1.0/m_fWavelength4[0], 1.0/m_fWavelength4[1], 1.0/m_fWavelength4[2]);
            pGroundShader->setFloat("fCameraHeight", glm::length(vCamera));
            pGroundShader->setFloat("fCameraHeight2", glm::length2(vCamera));
            pGroundShader->setFloat("fInnerRadius", m_fInnerRadius);
            pGroundShader->setFloat("fInnerRadius2", m_fInnerRadius*m_fInnerRadius);
            pGroundShader->setFloat("fOuterRadius", m_fOuterRadius);
            pGroundShader->setFloat("fOuterRadius2", m_fOuterRadius*m_fOuterRadius);
            pGroundShader->setFloat("fKrESun", m_Kr*m_ESun);
            pGroundShader->setFloat("fKmESun", m_Km*m_ESun);
            pGroundShader->setFloat("fKr4PI", m_Kr4PI);
            pGroundShader->setFloat("fKm4PI", m_Km4PI);
            pGroundShader->setFloat("fScale", 1.0f / (m_fOuterRadius - m_fInnerRadius));
            pGroundShader->setFloat("fScaleDepth", m_fRayleighScaleDepth);
            pGroundShader->setFloat("fScaleOverScaleDepth", (1.0f / (m_fOuterRadius - m_fInnerRadius)) / m_fRayleighScaleDepth);
            pGroundShader->setFloat("g", m_g);
            pGroundShader->setFloat("g2", m_g*m_g);
            pGroundShader->setFloat("fESun",m_ESun);
            pGroundShader->setInt("s2Tex1", 1);
            pGroundShader->setInt("s2Tex2", 2);
            pGroundShader->setInt("s2Tex3", 3);
            pGroundShader->setInt("opticalTex", 0);


            pGroundShader->setMat4("m4ModelViewProjectionMatrix",
                                   m_3DCamera.GetFrustumMatrix()*glm::scale(glm::mat4(1), glm::vec3(m_fInnerRadius))
                                   *glm::mat4(glm::quat(glm::radians(m_vRotation))) );
            pGroundShader->setMat4("m4ModelMatrix",
                                   glm::scale(glm::mat4(1), glm::vec3(m_fInnerRadius))
                                   *glm::mat4(glm::quat(glm::radians(m_vRotation))) );

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_tOpticalDepthBuffer);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture1);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, texture2);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, texture3);



            m_tEarth.draw();
        }

        {
            // Draw sky
            pSkyShader->use();
            pSkyShader->setVec3("v3CameraPos", vCamera.x, vCamera.y, vCamera.z);
            pSkyShader->setVec3("v3LightDir", m_vLightDirection);
            pSkyShader->setVec3("v3InvWavelength", 1/m_fWavelength4[0], 1/m_fWavelength4[1], 1/m_fWavelength4[2]);
            pSkyShader->setFloat("fCameraHeight", glm::length(vCamera));
            pSkyShader->setFloat("fCameraHeight2", glm::length2(vCamera));
            pSkyShader->setFloat("fInnerRadius", m_fInnerRadius);
            pSkyShader->setFloat("fInnerRadius2", m_fInnerRadius*m_fInnerRadius);
            pSkyShader->setFloat("fOuterRadius", m_fOuterRadius);
            pSkyShader->setFloat("fOuterRadius2", m_fOuterRadius*m_fOuterRadius);
            pSkyShader->setFloat("fKrESun", m_Kr*m_ESun);
            pSkyShader->setFloat("fKmESun", m_Km*m_ESun);
            pSkyShader->setFloat("fKr4PI", m_Kr4PI);
            pSkyShader->setFloat("fKm4PI", m_Km4PI);
            pSkyShader->setFloat("fScale", 1.0f / (m_fOuterRadius - m_fInnerRadius));
            pSkyShader->setFloat("fScaleDepth", m_fRayleighScaleDepth);
            pSkyShader->setFloat("fScaleOverScaleDepth", (1.0f / (m_fOuterRadius - m_fInnerRadius)) / m_fRayleighScaleDepth);
            pSkyShader->setFloat("g", m_g);
            pSkyShader->setFloat("g2", m_g*m_g);
            pSkyShader->setInt("opticalTex", 0);


            pSkyShader->setMat4("m4ModelViewProjectionMatrix",
                                m_3DCamera.GetFrustumMatrix()*glm::scale(glm::mat4(1), glm::vec3(m_fOuterRadius))
                                *glm::mat4(glm::quat(glm::radians(m_vRotation))) );
            pSkyShader->setMat4("m4ModelMatrix",
                                glm::scale(glm::mat4(1), glm::vec3(m_fOuterRadius))
                                *glm::mat4(glm::quat(glm::radians(m_vRotation))) );

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_tOpticalDepthBuffer);


            glFrontFace(GL_CW);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_tEarth.draw();
            glDisable(GL_BLEND);
            glFrontFace(GL_CCW);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        hdrShader.setInt("hdr", m_fHdr);
        hdrShader.setFloat("exposure", m_fExposure);
        renderQuad();

        GuiInterface::Begin();
        gui_interface();
        GuiInterface::End();

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate( );

    return 0;
}

bool countAndDisplayFps(GLFWwindow* window)
{
    float currentFrame = float(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    frameCount++;
    if(float(glfwGetTime()) - lastFpsCountFrame > 1.0f)
    {
        std::cout << "Current fps: "
                  << frameCount/(glfwGetTime() - lastFpsCountFrame)
                  << " runtime:"
                  << glfwGetTime()
                  << std::endl; // deprecated

        frameCount = 0;
        lastFpsCountFrame = float(glfwGetTime());
        return true;
    }
    if(deltaTime > 60.0f) {
        std::cout << "No response for 60 sec... exit program." << std::endl;
        glfwTerminate();
        EXIT_FAILURE;
    }
    return false;
}




// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
static unsigned int cubeVAO = 0;
static unsigned int cubeVBO = 0;
void renderBox()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // x-dir
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right

            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            // y dir
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right

            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
            // z dir
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right

            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,  // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f // top-right

        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 24);
    glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


static int key_space_old_state = GLFW_RELEASE;
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_3DCamera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_3DCamera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_3DCamera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_3DCamera.ProcessKeyboard(RIGHT, deltaTime);

    if ((glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) && (key_space_old_state == GLFW_RELEASE))
    {
        m_fHdr = !m_fHdr;
    }
    key_space_old_state = glfwGetKey(window, GLFW_KEY_SPACE);


    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        if (m_fExposure > 0.0f)
            m_fExposure -= 0.001f;
        else
            m_fExposure = 0.0f;
    }
    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        m_fExposure += 0.001f;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    if(!window) return;
    glViewport(0, 0, w, h);
    m_3DCamera.updateAspect(float(w) / float(h));
}

static bool mouse_button_right = false;
// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if(!window) return;
    if (firstMouse)
    {
        lastX = float(xpos);
        lastY = float(ypos);
        firstMouse = false;
    }

    float xoffset = float(xpos) - lastX;
    float yoffset = lastY - float(ypos); // reversed since y-coordinates go from bottom to top

    lastX = float(xpos);
    lastY = float(ypos);

    if(mouse_button_right)
        m_3DCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(!window) return;
    //m_3DCamera.ProcessMouseScroll(float(0*xoffset));
    m_3DCamera.ProcessMouseScroll(float(yoffset));
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(!window) return;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {mouse_button_right = true;return;}
    mouse_button_right = false;
    mods = 0;
}

GLFWwindow* initGL(int w, int h)
{
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        EXIT_FAILURE;
    }
    
    //glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if 0
    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
#endif

    // Open a window and create its OpenGL context
    GLFWwindow* window = glfwCreateWindow( w, h, "", nullptr, nullptr);
    if( window == nullptr ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        EXIT_FAILURE;
    }
    
    // Initial viewport
    glViewport(0, 0, w, h);
    
    // Query infomation
    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;
    std::cout << "Hardware: " <<glGetString(GL_RENDERER) << std::endl;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &nrAttributes);
    std::cout << "Maximum nr of color attachments supported: " << nrAttributes << std::endl;
    glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &nrAttributes);
    std::cout << "Maximum nr of image uniforms supported by fragment shader: " << nrAttributes << std::endl;
    glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &nrAttributes);
    std::cout << "Maximum nr of image uniforms supported by compute shader: " << nrAttributes << std::endl;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &nrAttributes);
    std::cout << "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " << nrAttributes << " bytes." << std::endl;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &nrAttributes);
    std::cout << "Maximum nr of shader storage buffer binding points is " << nrAttributes << " ." << std::endl;
    
    // Compute Shader Configuration
    int work_grp_cnt[3], work_grp_inv;

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

    printf("max global (total) work group size x:%i y:%i z:%i\n",
           work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    printf("max local work group invocations %i\n", work_grp_inv);


    // framebuffer mode
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // Mouse input mode
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSwapInterval(1); // 60 fps constraint
    
    return window;
}
