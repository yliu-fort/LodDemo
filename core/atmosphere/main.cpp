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
//#include "Icosphere.h"

#define PI (3.141592654)

// settings
static int SCR_WIDTH  = 800;
static int SCR_HEIGHT = 600;

// camera
static Camera m_3DCamera = Camera(glm::vec3(0.0f, 0.0f, 25.0f), float(SCR_WIDTH)/SCR_HEIGHT);
static auto m_vLight = glm::vec3(0, 0, 1000);
static auto m_vLightDirection = glm::normalize(m_vLight);

static auto m_nSamples = 3;		// Number of sample rays to use in integral equation
static auto m_Kr = 0.0025f;		// Rayleigh scattering constant
static auto m_Kr4PI = m_Kr*4.0f*PI;
static auto m_Km = 0.0010f;		// Mie scattering constant
static auto m_Km4PI = m_Km*4.0f*PI;
static auto m_ESun = 20.0f;		// Sun brightness constant
static auto m_g = -0.990f;		// The Mie phase asymmetry factor
static auto m_fExposure = 2.0f;

static auto m_fInnerRadius = 10.0f;
static auto m_fOuterRadius = 10.25f;
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

static auto m_fRayleighScaleDepth = 0.25f;
static auto m_fMieScaleDepth = 0.1f;



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

#define VISUAL


// Shortcut
static bool pause = true;

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

    Shader m_shSkyFromSpace         (FP("atmosphereRender/SkyFromSpace.vert"          ),FP("atmosphereRender/SkyFromSpace.frag"         ));
    Shader m_shSkyFromAtmosphere    (FP("atmosphereRender/SkyFromAtmosphere.vert"     ),FP("atmosphereRender/SkyFromAtmosphere.frag"    ));
    Shader m_shGroundFromSpace      (FP("atmosphereRender/GroundFromSpace.vert"       ),FP("atmosphereRender/GroundFromSpace.frag"      ));
    Shader m_shGroundFromAtmosphere (FP("atmosphereRender/GroundFromAtmosphere.vert"  ),FP("atmosphereRender/GroundFromAtmosphere.frag" ));
    //Shader m_shSpaceFromSpace       (FP("atmosphereRender/SpaceFromSpace.vert"        ),FP("atmosphereRender/SpaceFromSpace.frag"       ));
    //Shader m_shSpaceFromAtmosphere  (FP("atmosphereRender/SpaceFromAtmosphere.vert"   ),FP("atmosphereRender/SpaceFromAtmosphere.frag"  ));

    // For automatic file reloading
    //FileSystemMonitor::Init(SRC_PATH);

    // Prepare buffers
    Icosphere sphere(6);

    // load textures
    // -------------
    uint texture1 = loadTexture("2k_earth_daymap.jpg", FP("../../resources/textures/earth"));
    //uint texture2 = loadTiffTexture("2k_earth_normal_map.tif", FP("../../resources/textures/earth"));
    //uint texture3 = loadTiffTexture("2k_earth_specular_map.tif", FP("../../resources/textures/earth"));

    //Store the volume data to polygonise
    uint texture2 = 0;
    glGenTextures(1, &texture2);
    glActiveTexture(GL_TEXTURE0);
    //glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //Generate a distance field to the center of the cube
    std::vector<glm::vec3> dataField;
    uint res = 4;
    for(uint j=0; j<res; j++){
        for(uint i=0; i<res; i++){
            dataField.push_back( glm::vec3(0,0,1) );
        }
    }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, res, res, 0, GL_RGB, GL_FLOAT, &dataField[0]);


    uint texture3 = 0;
    glGenTextures(1, &texture3);
    glActiveTexture(GL_TEXTURE0);
    //glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //Generate a distance field to the center of the cube
    dataField.clear();
    for(uint j=0; j<res; j++){
        for(uint i=0; i<res; i++){
            dataField.push_back( glm::vec3(0,0,0) );
        }
    }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, res, res, 0, GL_RGB, GL_FLOAT, &dataField[0]);


    while( !glfwWindowShouldClose( window ) )
    {
        // per-frame time logic
        // --------------------
        countAndDisplayFps(window);

#ifdef VISUAL
        // input
        glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
        processInput(window);

        // Draw points
        glViewport(0,0,SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //lightShader.use();
        //
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, texture1);
        //lightShader.setInt("diffuseMap",0);
        //lightShader.setInt("heightMap",0);
        //
        //glActiveTexture(GL_TEXTURE1);
        //glBindTexture(GL_TEXTURE_2D, texture2);
        //lightShader.setInt("normalMap",1);
        //
        //glActiveTexture(GL_TEXTURE2);
        //glBindTexture(GL_TEXTURE_2D, texture3);
        //lightShader.setInt("specularMap",2);
        //
        //lightShader.setMat4("projection_view", m_3DCamera.GetFrustumMatrix());
        //lightShader.setMat4("model", glm::scale(glm::mat4(1), glm::vec3(10)));
        //lightShader.setVec3("viewPos",m_3DCamera.Position);
        //lightShader.setVec3("lightPos", glm::vec3(-0.25f,-0.23f,100.2f));

        //lineShader.use();
        //lineShader.setVec3("color",glm::vec3(1));
        //lineShader.setMat4("projectionMatrix", camera.GetFrustumMatrix());
        //
        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        //sphere.draw();
        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

        //renderBox();


        auto& vCamera = m_3DCamera.Position;
        Shader *pGroundShader, *pSkyShader;
        if(glm::length(vCamera) >= m_fOuterRadius)
        {
            pGroundShader = &m_shGroundFromSpace;
            pSkyShader = &m_shSkyFromSpace;
        }
        else
        {
            pGroundShader = &m_shGroundFromAtmosphere;
            pSkyShader = &m_shSkyFromAtmosphere;
        }

        {
            // Draw ground
            pGroundShader->use();
            pGroundShader->setVec3("v3CameraPos", vCamera.x, vCamera.y, vCamera.z);
            pGroundShader->setVec3("v3LightPos", m_vLightDirection.x, m_vLightDirection.y, m_vLightDirection.z);
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
            pGroundShader->setInt("s2Tex1", 0);

            pGroundShader->setMat4("m4ModelViewProjectionMatrix",
                                   m_3DCamera.GetFrustumMatrix()*glm::scale(glm::mat4(1), glm::vec3(m_fInnerRadius)));
            pGroundShader->setMat4("m4ModelMatrix",
                                   glm::scale(glm::mat4(1), glm::vec3(m_fInnerRadius)));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);

            sphere.draw();
        }

        {
            // Draw sky
            pSkyShader->use();
            pSkyShader->setVec3("v3CameraPos", vCamera.x, vCamera.y, vCamera.z);
            pSkyShader->setVec3("v3LightPos", m_vLightDirection.x, m_vLightDirection.y, m_vLightDirection.z);
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

            pSkyShader->setMat4("m4ModelViewProjectionMatrix",
                                m_3DCamera.GetFrustumMatrix()*glm::scale(glm::mat4(1), glm::vec3(m_fOuterRadius)));
            pSkyShader->setMat4("m4ModelMatrix",
                                glm::scale(glm::mat4(1), glm::vec3(m_fOuterRadius)));


            glFrontFace(GL_CW);
            //glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            sphere.draw();
            //glDisable(GL_BLEND);
            glFrontFace(GL_CCW);
        }

#endif

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
#ifdef VISUAL



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
        pause = !pause;
    }
    key_space_old_state = glfwGetKey(window, GLFW_KEY_SPACE);
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
#endif

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
#ifndef VISUAL
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

#ifdef VISUAL
    // framebuffer mode
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // Mouse input mode
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSwapInterval(1); // 60 fps constraint
#else
    glfwSwapInterval(0); // No fps constraint
#endif
    
    return window;
}
