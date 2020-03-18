#include <iostream>
#include <cmath>

//GLEW
#define GLEW_STATIC
#include <GL/glew.h>

//GLFW
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "cmake_source_dir.h"
#include "shader.h"
#include "camera.h"
#include "colormap.h"
#include "filesystemmonitor.h"
//#include "gui_interface.h"
//#include "slice.h"
//#include "boundingbox.h"
#include "image_io.h"
#include "texture_utility.h"

// settings
static int SCR_WIDTH  = 800;
static int SCR_HEIGHT = 600;

// camera
static Camera camera = Camera(glm::vec3(0.5f, 0.5f, 0.5f), float(SCR_WIDTH)/SCR_HEIGHT);

static float lastX = SCR_WIDTH / 2.0f;
static float lastY = SCR_HEIGHT / 2.0f;
static bool firstMouse = true;

// timing
static float deltaTime = 0.0f;
static float lastFrame = 0.0f;

// fps recording
static float lastFpsCountFrame = 0;
static int frameCount = 0;

// Shortcut
static bool bindCam = true;

// Pre-declaration
GLFWwindow* initGL(int w, int h);
bool countAndDisplayFps(GLFWwindow* window);
void processInput(GLFWwindow *window);
void renderBox();
void renderPlane();
void renderGrid();
#define VISUAL

#define HEIGHT_MAP_X (17)
#define HEIGHT_MAP_Y (17)

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

    // Read shader
    //Shader shader(FP("renderer/terrain.vert"),FP("renderer/terrain.frag"));
    //Shader pColorShader(FP("renderer/box.vert"),FP("renderer/box.frag"));
    //Shader lodShader(FP("renderer/lod.vert"),FP("renderer/lod.frag"));
    Shader tex_interp(FP("renderer/tex_interp.glsl"));
    Shader lodShader(FP("../quadtree/renderer/lod.vert"),FP("../quadtree/renderer/lod.pcolor.frag"));

    // For automatic file reloading
    FileSystemMonitor::Init(SRC_PATH);

    // Initialize 2d noise texture
    //Datafield//

    //Store the volume data to polygonise
    unsigned int largeMap;
    glGenTextures(1, &largeMap);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, largeMap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //Generate a distance field to the center of the cube
    std::vector<float> dataField;

    for(uint j=0; j<HEIGHT_MAP_Y; j++){
        for(uint i=0; i<HEIGHT_MAP_X; i++){
            dataField.push_back(0.001f*rand() / double(RAND_MAX)*(i+HEIGHT_MAP_X*j));
            //dataField.push_back(i+HEIGHT_MAP_X*j);
        }
    }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_R32F, HEIGHT_MAP_X, HEIGHT_MAP_Y, 0, GL_RED, GL_FLOAT, &dataField[0]);


    //Datafield//
    //Store the volume data to polygonise
    unsigned int smallMap = 0;
    glGenTextures(1,&smallMap);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, smallMap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //Generate a distance field to the center of the cube
    std::vector<float>().swap(dataField);
    for(uint j=0; j<HEIGHT_MAP_Y; j++){
        for(uint i=0; i<HEIGHT_MAP_X; i++){
            dataField.push_back(0);
        }
    }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_R32F, HEIGHT_MAP_X, HEIGHT_MAP_Y, 0, GL_RED, GL_FLOAT, &dataField[0]);

    // Geo mesh, careful: need a noise texture and shader before intialized

    std::vector<float> small(HEIGHT_MAP_X*HEIGHT_MAP_Y);
    std::vector<float> large(HEIGHT_MAP_X*HEIGHT_MAP_Y);

    // read texture
    glGetTextureImage(smallMap, 0, GL_RED, GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&small[0]);

    // read texture
    glGetTextureImage(largeMap, 0, GL_RED, GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&large[0]);


    printf("Pixel value in small map: \n");
    for(uint i=0; i<HEIGHT_MAP_X*HEIGHT_MAP_Y; i++){
        printf("%.1f ", small[i]);
        if(i%HEIGHT_MAP_X == HEIGHT_MAP_X-1) {printf("\n");}
    }

    printf("Pixel value in large map: \n");
    for(uint i=0; i<HEIGHT_MAP_X*HEIGHT_MAP_Y; i++){
        printf("%.1f ", large[i]);
        if(i%HEIGHT_MAP_X == HEIGHT_MAP_X-1) {printf("\n");}
    }


    // Interpolate
    // Compute
    tex_interp.use();

    // bind noisemap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, largeMap);

    // write to heightmap
    glBindImageTexture(0, smallMap, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    // Deploy kernel
    glDispatchCompute(1,1,1);

    // make sure writing to image has finished before read
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


    // read texture
    glGetTextureImage(smallMap, 0, GL_RED, GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&small[0]);

    // read texture
    glGetTextureImage(largeMap, 0, GL_RED, GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&large[0]);


    printf("Pixel value in small map: \n");
    for(uint i=0; i<HEIGHT_MAP_X*HEIGHT_MAP_Y; i++){
        printf("%.1f ", small[i]);
        if(i%HEIGHT_MAP_X == HEIGHT_MAP_X-1) {printf("\n");}
    }

    printf("Pixel value in large map: \n");
    for(uint i=0; i<HEIGHT_MAP_X*HEIGHT_MAP_Y; i++){
        printf("%.1f ", large[i]);
        if(i%HEIGHT_MAP_X == HEIGHT_MAP_X-1) {printf("\n");}
    }

    while( !glfwWindowShouldClose( window ) )
    {
        // per-frame time logic
        // --------------------
        bool ticking = countAndDisplayFps(window);

#ifdef VISUAL
        // input
        glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
        processInput(window);

        glViewport(0,0,SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        lodShader.use();
        lodShader.setMat4("projection_view", camera.GetFrustumMatrix());
        //lodShader.setVec3("viewPos", camera.Position);
        lodShader.setInt("heightmap", 0);

        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

        // grid 1
        lodShader.setVec3("color", glm::vec3(0.2,0.2,0.7));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, largeMap);
        lodShader.setMat4("model", glm::mat4(1));
        renderGrid();

        // grid 2
        lodShader.setVec3("color", glm::vec3(0.2,0.7,0.2));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, smallMap);
        lodShader.setMat4("model", glm::translate(glm::scale(glm::mat4(1),glm::vec3(0.5,1,0.5)),glm::vec3(-1,0,0)));
        renderGrid();

        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );


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
static int key_space_old_state = GLFW_RELEASE;
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(COUNTERCLOCKWISE, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(CLOCKWISE, deltaTime);

    if ((glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) && (key_space_old_state == GLFW_RELEASE))
    {
        bindCam = !bindCam;
        camera.printInfo();
    }
    key_space_old_state = glfwGetKey(window, GLFW_KEY_SPACE);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    if(!window) return;
    glViewport(0, 0, w, h);
    camera.updateAspect(float(w) / float(h));
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
        camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if(!window) return;
    //camera.ProcessMouseScroll(float(0*xoffset));
    camera.ProcessMouseScroll(float(yoffset));
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
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
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
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
static unsigned int planeVAO = 0;
static unsigned int planeVBO = 0;
void renderPlane()
{
    // initialize (if necessary)
    if (planeVAO == 0)
    {
        float vertices[] = {
            // z dir
            1.0f, 0.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, 0.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, 0.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, 0.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right

        };
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(planeVAO);
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
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


// renderGrid() renders a 16x16 2d grid in NDC.
// -------------------------------------------------
static unsigned int gridVAO = 0;
static unsigned int gridVBO = 0;
#define GRIDX (16)
#define GRIDY (16)
void renderGrid()
{

    // initialize (if necessary)
    if (gridVAO == 0)
    {
        struct float8
        {
            float val[8];
            float8(float  v1, float v2, float v3, float v4,float  v5, float v6, float v7, float v8)
            {val[0] = v1;val[1] = v2;val[2] = v3;val[3] = v4;val[4] = v5;val[5] = v6;val[6] = v7;val[7] = v8;}
        };
        std::vector<float8> vertices;

        for(int j = 0; j < GRIDY; j++)
            for(int i = 0; i < GRIDX; i++)
            {
                glm::vec2 lo(    i/float(GRIDX),    j/float(GRIDY));
                glm::vec2 hi((i+1)/float(GRIDX),(j+1)/float(GRIDY));

                vertices.push_back(float8( lo.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, lo.x, lo.y)); // bottom-left
                vertices.push_back(float8( lo.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, lo.x, hi.y)); // top-left
                vertices.push_back(float8( hi.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, hi.x, hi.y)); // top-right

                vertices.push_back(float8( hi.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, hi.x, hi.y)); // top-right
                vertices.push_back(float8( hi.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, hi.x, lo.y)); // bottom-right
                vertices.push_back(float8( lo.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, lo.x, lo.y)); // bottom-left

            }

        std::cout <<  std::endl;

        glGenVertexArrays(1, &gridVAO);
        glGenBuffers(1, &gridVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, 8*sizeof(float)*vertices.size(), &vertices[0], GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(gridVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Grid
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6*GRIDX*GRIDY);
    glBindVertexArray(0);
}
