#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include <iostream>
#include <cmath>

#include "shader.h"
#include "camera.h"
//GLFW
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "geocube.h"

//#include "texture_utility.h"
//#include "icosphere.h"

//#include "gui_interface.h"
//#include "imgui.h"

#define PI (3.141592654)

class Atmosphere
{

public:
    Atmosphere(Camera& cam) : m_3DCamera(cam) {}
    ~Atmosphere()
    {
        //glDeleteTextures(1,&m_tPhaseBuffer);
        //glDeleteTextures(1,&m_tOpticalDepthBuffer);
    }

    void bindCamera(Camera& cam) { m_3DCamera = cam; }
    void init();
    void drawGround();
    void drawSky();
    void MakeOpticalDepthBuffer(float fInnerRadius, float fOuterRadius, float fRayleighScaleHeight, float fMieScaleHeight);
    void MakePhaseBuffer(float ESun, float Kr, float Km, float g);
    void update();
    void reset();
    void gui_interface();

protected:
    Shader& getGroundShader(const glm::vec3& pos);
    Shader& getSkyShader(const glm::vec3& pos);


private:
    Camera& m_3DCamera;
    glm::vec3 m_vLight = glm::vec3(0, 0, 1000);
    glm::vec3 m_vLightDirection = glm::normalize(m_vLight);
    glm::vec3 m_vRotation = glm::vec3(0, 0, 0);

    int m_nSamples = 3;		// Number of sample rays to use in integral equation
    float m_Kr = 0.0025f;		// Rayleigh scattering constant
    float m_Kr4PI = m_Kr*4.0f*PI;
    float m_Km = 0.001f;		// Mie scattering constant
    float m_Km4PI = m_Km*4.0f*PI;
    float m_ESun = 15.0f;		// Sun brightness constant
    ////For Mie aerosol scattering, g is usually set between -0.75 and -0.999
    float m_g = -0.990f;		// The Mie phase asymmetry factor
    float m_fExposure = 1.0f;

    float m_fInnerRadius = 1.0f;
    float m_fOuterRadius = 1.0126f;
    float m_fScale = 1 / (m_fOuterRadius - m_fInnerRadius);

    float m_fWavelength[3]{
        0.700f,     // 650 nm for red
        0.546f,     // 570 nm for green
        0.435f      // 475 nm for blue
    };

    float m_fWavelength4[3] {
        powf(m_fWavelength[0], 4.0f),
                powf(m_fWavelength[1], 4.0f),
                powf(m_fWavelength[2], 4.0f)
    };

    ////My implementation uses 0.25, so the average density is found 25 percent of the way up from the ground to the sky dome.
    float m_fRayleighScaleDepth = 0.1f;
    float m_fMieScaleDepth = 0.1f;
    bool m_fHdr = true;

    unsigned int m_tOpticalDepthBuffer, m_tPhaseBuffer;
    int m_nODBSize = 256;
    int m_nODBSamples = 50;

    Shader m_shSkyFromSpace         ;
    Shader m_shSkyFromAtmosphere    ;
    Shader m_shGroundFromSpace      ;
    Shader m_shGroundFromAtmosphere ;
    Shader m_shSpaceFromSpace       ;
    Shader m_shSpaceFromAtmosphere  ;

    Geocube m_tEarth, m_tSky;
};

#endif
