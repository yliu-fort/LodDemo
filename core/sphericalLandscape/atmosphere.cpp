#include "atmosphere.h"

#include <glad/glad.h>

#include "cmake_source_dir.h"

void Atmosphere::init()
{
    m_shSkyFromSpace         .reload_shader_program_from_files(FP("renderer/atmosphere/SkyFromSpace.vert"          ),FP("renderer/atmosphere/SkyFromSpace.frag"         ));
    m_shSkyFromAtmosphere    .reload_shader_program_from_files(FP("renderer/atmosphere/SkyFromAtmosphere.vert"     ),FP("renderer/atmosphere/SkyFromAtmosphere.frag"    ));
    m_shGroundFromSpace      .reload_shader_program_from_files(FP("renderer/atmosphere/GroundFromSpace.vert"       ),FP("renderer/atmosphere/GroundFromSpace.frag"      ));
    m_shGroundFromAtmosphere .reload_shader_program_from_files(FP("renderer/atmosphere/GroundFromAtmosphere.vert"  ),FP("renderer/atmosphere/GroundFromAtmosphere.frag" ));
    //m_shSpaceFromSpace       .reload_shader_program_from_files(FP("renderer/atmosphere/SpaceFromSpace.vert"        ),FP("renderer/atmosphere/SpaceFromSpace.frag"       ));
    //m_shSpaceFromAtmosphere  .reload_shader_program_from_files(FP("renderer/atmosphere/SpaceFromAtmosphere.vert"   ),FP("renderer/atmosphere/SpaceFromAtmosphere.frag"  ));

    m_tEarth = Geocube();

    m_tSky = Geocube();
    m_tSky.subdivision(3);
    m_tSky.releaseAllTextureHandles();

    update();
}

Shader& Atmosphere::getGroundShader(const glm::vec3& pos)
{
    if(glm::length(pos) >= m_fOuterRadius)
    {
        return m_shGroundFromSpace;
    }
    else
    {
        return m_shGroundFromAtmosphere;
    }
}

Shader& Atmosphere::getSkyShader(const glm::vec3& pos)
{
    if(glm::length(pos) >= m_fOuterRadius)
    {
        return m_shSkyFromSpace;
    }
    else
    {
        return m_shSkyFromAtmosphere;
    }
}

void Atmosphere::drawGround(Camera& camera)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);

    // get reference
    const auto& vCamera = m_3DCamera.Position;
    Shader& pGroundShader = getGroundShader(vCamera);

    // Draw ground
    pGroundShader.use();
    pGroundShader.setVec3("v3CameraPos", vCamera.x, vCamera.y, vCamera.z);
    pGroundShader.setVec3("v3LightDir", m_vLightDirection);
    pGroundShader.setVec3("v3InvWavelength", 1.0/m_fWavelength4[0], 1.0/m_fWavelength4[1], 1.0/m_fWavelength4[2]);
    pGroundShader.setFloat("fCameraHeight", glm::length(vCamera));
    pGroundShader.setFloat("fCameraHeight2", glm::length2(vCamera));
    pGroundShader.setFloat("fInnerRadius", m_fInnerRadius);
    pGroundShader.setFloat("fInnerRadius2", m_fInnerRadius*m_fInnerRadius);
    pGroundShader.setFloat("fOuterRadius", m_fOuterRadius);
    pGroundShader.setFloat("fOuterRadius2", m_fOuterRadius*m_fOuterRadius);
    pGroundShader.setFloat("fKrESun", m_Kr*m_ESun);
    pGroundShader.setFloat("fKmESun", m_Km*m_ESun);
    pGroundShader.setFloat("fKr4PI", m_Kr4PI);
    pGroundShader.setFloat("fKm4PI", m_Km4PI);
    pGroundShader.setFloat("fScale", 1.0f / (m_fOuterRadius - m_fInnerRadius));
    pGroundShader.setFloat("fScaleDepth", m_fRayleighScaleDepth);
    pGroundShader.setFloat("fScaleOverScaleDepth", (1.0f / (m_fOuterRadius - m_fInnerRadius)) / m_fRayleighScaleDepth);
    pGroundShader.setFloat("g", m_g);
    pGroundShader.setFloat("g2", m_g*m_g);
    pGroundShader.setFloat("fESun",m_ESun);
    pGroundShader.setInt("heightmap", 0);
    pGroundShader.setInt("heightmapParent", 1);
    pGroundShader.setInt("s2Tex1", 2);
    pGroundShader.setInt("s2Tex2", 3);
    pGroundShader.setInt("opticalTex", 4);
    pGroundShader.setInt("s2TexTest", 10);



    pGroundShader.setMat4("m4ModelViewProjectionMatrix",
                           m_3DCamera.GetFrustumMatrix() );

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, m_tOpticalDepthBuffer);

    m_tEarth.draw(pGroundShader, camera);
}

void Atmosphere::drawSky(Camera& camera)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);

    // get reference
    const auto& vCamera = m_3DCamera.Position;
    Shader& pSkyShader = getSkyShader(vCamera);

    {
        // Draw sky
        pSkyShader.use();
        pSkyShader.setVec3("v3CameraPos", vCamera.x, vCamera.y, vCamera.z);
        pSkyShader.setVec3("v3LightDir", m_vLightDirection);
        pSkyShader.setVec3("v3InvWavelength", 1.0/m_fWavelength4[0], 1.0/m_fWavelength4[1], 1.0/m_fWavelength4[2]);
        pSkyShader.setFloat("fCameraHeight", glm::length(vCamera));
        pSkyShader.setFloat("fCameraHeight2", glm::length2(vCamera));
        pSkyShader.setFloat("fInnerRadius", m_fInnerRadius);
        pSkyShader.setFloat("fInnerRadius2", m_fInnerRadius*m_fInnerRadius);
        pSkyShader.setFloat("fOuterRadius", m_fOuterRadius);
        pSkyShader.setFloat("fOuterRadius2", m_fOuterRadius*m_fOuterRadius);
        pSkyShader.setFloat("fKrESun", m_Kr*m_ESun);
        pSkyShader.setFloat("fKmESun", m_Km*m_ESun);
        pSkyShader.setFloat("fKr4PI", m_Kr4PI);
        pSkyShader.setFloat("fKm4PI", m_Km4PI);
        pSkyShader.setFloat("fScale", 1.0f / (m_fOuterRadius - m_fInnerRadius));
        pSkyShader.setFloat("fScaleDepth", m_fRayleighScaleDepth);
        pSkyShader.setFloat("fScaleOverScaleDepth", (1.0f / (m_fOuterRadius - m_fInnerRadius)) / m_fRayleighScaleDepth);
        pSkyShader.setFloat("g", m_g);
        pSkyShader.setFloat("g2", m_g*m_g);
        pSkyShader.setInt("opticalTex", 4);


        pSkyShader.setMat4("m4ModelViewProjectionMatrix",
                            m_3DCamera.GetFrustumMatrix() );

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, m_tOpticalDepthBuffer);


        glFrontFace(GL_CW);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        m_tSky.draw(pSkyShader, camera);
        glDisable(GL_BLEND);
        glFrontFace(GL_CCW);
    }
}

void Atmosphere::MakeOpticalDepthBuffer(float fInnerRadius, float fOuterRadius, float fRayleighScaleHeight, float fMieScaleHeight)
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
        std::vector<float> rd;
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
            (m_pBuffer)[nIndex++] = /*fMieDepth*/ 1.0;

            //rd.push_back(fRayleighDepth);

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

void Atmosphere::MakePhaseBuffer(float ESun, float Kr, float Km, float g)
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

void Atmosphere::update()
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


    m_tEarth.setScale(m_fInnerRadius);
    m_tSky.setScale(m_fOuterRadius);

}

void Atmosphere::reset()
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

    m_fWavelength[0] = 0.700f;
    m_fWavelength[1] = 0.546f;
    m_fWavelength[2] = 0.435f;

    m_fRayleighScaleDepth = 0.1f;
    m_fMieScaleDepth = 0.1f;
    m_fHdr = true;

    update();
}

#include "imgui.h"
void Atmosphere::gui_interface()
{
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Atmosphere::Control Panel"))
    {
        ImGui::Text("Controllable parameters for Atmosphere class.");

        // Transform
        ImGui::DragFloat3("Camera Position",&(m_3DCamera.Position)[0], 0.0001,1.0f,99.0f,"%.6f");
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

        if (ImGui::TreeNode("Scatter buffer"))
        {
            ImGuiIO& io = ImGui::GetIO();
            //ImGui::TextWrapped("Below we are displaying the font texture (which is the only texture we have access to in this demo). Use the 'ImTextureID' type as storage to pass pointers or identifier to your own texture data. Hover the texture for a zoomed view!");

            // Here we are grabbing the font texture because that's the only one we have access to inside the demo code.
            // Remember that ImTextureID is just storage for whatever you want it to be, it is essentially a value that will be passed to the render function inside the ImDrawCmd structure.
            // If you use one of the default imgui_impl_XXXX.cpp renderer, they all have comments at the top of their file to specify what they expect to be stored in ImTextureID.
            // (for example, the imgui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer. The imgui_impl_opengl3.cpp renderer expect a GLuint OpenGL texture identifier etc.)
            // If you decided that ImTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers to ImGui::Image(), and gather width/height through your own functions, etc.
            // Using ShowMetricsWindow() as a "debugger" to inspect the draw data that are being passed to your render will help you debug issues if you are confused about this.
            // Consider using the lower-level ImDrawList::AddImage() API, via ImGui::GetWindowDrawList()->AddImage().
            ImTextureID my_tex_id = (GLuint*)m_tOpticalDepthBuffer;
            float my_tex_w = (float)256;
            float my_tex_h = (float)256;

            ImGui::Text("Rayleigh Density");
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::Image(my_tex_id, ImVec2(my_tex_w, my_tex_h), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,0.0f,0.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                float region_sz = 32.0f;
                float region_x = io.MousePos.x - pos.x - region_sz * 0.5f; if (region_x < 0.0f) region_x = 0.0f; else if (region_x > my_tex_w - region_sz) region_x = my_tex_w - region_sz;
                float region_y = io.MousePos.y - pos.y - region_sz * 0.5f; if (region_y < 0.0f) region_y = 0.0f; else if (region_y > my_tex_h - region_sz) region_y = my_tex_h - region_sz;
                float zoom = 4.0f;
                ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
                ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
                ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
                ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
                ImGui::Image(my_tex_id, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::EndTooltip();
            }

            ImGui::Text("Rayleigh Optical Depth");
            pos = ImGui::GetCursorScreenPos();
            ImTextureID my_tex_id2 = (GLuint*)m_tOpticalDepthBuffer;
            ImGui::Image(my_tex_id2, ImVec2(my_tex_w, my_tex_h), ImVec2(0,0), ImVec2(1,1), ImVec4(0.0f,1.0f,0.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                float region_sz = 32.0f;
                float region_x = io.MousePos.x - pos.x - region_sz * 0.5f; if (region_x < 0.0f) region_x = 0.0f; else if (region_x > my_tex_w - region_sz) region_x = my_tex_w - region_sz;
                float region_y = io.MousePos.y - pos.y - region_sz * 0.5f; if (region_y < 0.0f) region_y = 0.0f; else if (region_y > my_tex_h - region_sz) region_y = my_tex_h - region_sz;
                float zoom = 4.0f;
                ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
                ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
                ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
                ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
                ImGui::Image(my_tex_id2, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::EndTooltip();
            }

            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

}
