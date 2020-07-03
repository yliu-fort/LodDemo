#ifndef PAtmosphere_H
#define PAtmosphere_H

#include <iostream>
#include <cmath>

#include "shader.h"
#include "camera.h"
//GLFW
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "OGeocube.h"

#define PI (3.141592654)

class PAtmosphere
{

public:
    PAtmosphere(Camera* cam) : m_3DCamera(cam) {}
    ~PAtmosphere()
    {
        //glDeleteTextures(1,&m_tPhaseBuffer);
        //glDeleteTextures(1,&m_tOpticalDepthBuffer);
    }

    void BindCamera(Camera* cam) { m_3DCamera = cam; }
    void Init();
    OGeocube& GetGroundHandle() {return m_tEarth;}
    OGeocube& GetSkyHandle() {return m_tSky;}
    OGeocube& GetOceanHandle() {return m_tOcean;}
    void DrawGround(Camera* camera);
    void DrawSky(Camera* camera);
    void DrawOcean(Camera* camera);
    void MakeOpticalDepthBuffer(float fInnerRadius, float fOuterRadius, float fRayleighScaleHeight, float fMieScaleHeight);
    void MakePhaseBuffer(float ESun, float Kr, float Km, float g);
    void Update();
    void Reset();
    void GuiInterface();
    inline void SetHDR(Shader& hdrShader)
    {
        hdrShader.setInt("hdr", m_fHdr);
        hdrShader.setFloat("exposure", m_fExposure);
    }
    inline float GetInnerRadius() const { return this->m_fInnerRadius; }
    inline float GetOuterRadius() const { return this->m_fOuterRadius; }
    inline bool InPAtmosphere(const glm::vec3& pos) const
    {
        if(glm::length(pos) >= this->GetOuterRadius())
            return false;
        return true;
    }

protected:
    Shader& GetGroundShader(const glm::vec3& pos);
    Shader& GetSkyShader(const glm::vec3& pos);
    Shader& GetOceanShader(const glm::vec3& pos);

private:
    Camera* m_3DCamera;
    glm::vec3 m_vLight = glm::vec3(0, 0, 1000);
    glm::vec3 m_vLightDirection = glm::normalize(m_vLight);

    int m_nSamples = 3;		// Number of sample rays to use in integral equation
    float m_Kr = 0.0025f;		// Rayleigh scattering constant
    float m_Kr4PI = m_Kr*4.0f*PI;
    float m_Km = 0.001f;		// Mie scattering constant
    float m_Km4PI = m_Km*4.0f*PI;
    float m_ESun = 20.0f;		// Sun brightness constant
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
    Shader m_shSkyFromPAtmosphere    ;
    Shader m_shGroundFromSpace      ;
    Shader m_shGroundFromPAtmosphere ;
    Shader m_shSpaceFromSpace       ;
    Shader m_shSpaceFromPAtmosphere  ;
    Shader m_shOceanFromSpace       ;
    Shader m_shOceanFromPAtmosphere  ;

    OGeocube m_tEarth, m_tSky, m_tOcean;
};

#endif
