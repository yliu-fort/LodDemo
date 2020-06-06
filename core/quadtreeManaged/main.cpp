#include <iostream>
#include <cmath>
#include <memory>

#include <glad/glad.h>

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
#include "gui_interface.h"
#include "imgui.h"
//#include "image_io.h"
#include "texture_utility.h"

#include "grid.h"
#include "geomesh.h"
#include "refcamera.h"
#include "lighting.h"

// settings
static int SCR_WIDTH  = 800;
static int SCR_HEIGHT = 600;

// camera
static Camera camera = Camera(glm::vec3(-1.0f, 0.0f, 2.0f), float(SCR_WIDTH)/SCR_HEIGHT);

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
static bool drawWireframe = false;
static bool drawNormalArrows = false;

// Pre-declaration
GLFWwindow* initGL(int w, int h);
bool countAndDisplayFps(GLFWwindow* window);
void processInput(GLFWwindow *window);
void renderBox();
void renderPlane();


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

class Geocube
{
    Geomesh top, bottom, left, right, front, back;

    glm::vec3 position,rotation;
    float scale;
    bool spinning = false;
    float spin_vel = 0.1f;

public:
    Geocube():position(0),rotation(0),scale(1)
        ,top(Geomesh(glm::translate(glm::mat4(1),glm::vec3(0,1,0))))
        ,bottom(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(180.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))))
        ,left(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(90.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))))
        ,right(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(-90.0f),glm::vec3(0,0,1)),glm::vec3(0,1,0))))
        ,front(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(90.0f),glm::vec3(1,0,0)),glm::vec3(0,1,0))))
        ,back(Geomesh(glm::translate(glm::rotate(glm::mat4(1),glm::radians(-90.0f),glm::vec3(1,0,0)),glm::vec3(0,1,0))))
    {}
    void update(Camera& camera)
    {
        self_spin();

        auto localPos = convertToLocal(camera.Position);
        top.subdivision(    localPos );
        bottom.subdivision( localPos );
        left.subdivision(   localPos );
        right.subdivision(  localPos );
        front.subdivision(  localPos );
        back.subdivision(   localPos );
    }
    void draw(Shader& shader, Camera& camera)
    {
        auto localPos = convertToLocal(camera.Position);
        shader.setMat4("model",this->getModelMatrix());
        top.draw(shader,    localPos );
        bottom.draw(shader, localPos );
        left.draw(shader,   localPos );
        right.draw(shader,  localPos );
        front.draw(shader,  localPos );
        back.draw(shader,   localPos );
    }
    float currentElevation(const glm::vec3& pos) const
    {
        auto localPos = convertToLocal(pos);
        if(top.isGroundReference(localPos))
            return top.queryElevation(localPos);
        if(bottom.isGroundReference(localPos))
            return bottom.queryElevation(localPos);
        if(left.isGroundReference(localPos))
            return left.queryElevation(localPos);
        if(right.isGroundReference(localPos))
            return right.queryElevation(localPos);
        if(front.isGroundReference(localPos))
            return front.queryElevation(localPos);
        if(back.isGroundReference(localPos))
            return back.queryElevation(localPos);
        return 0.0f;
    }
    float currentLocalHeight(const glm::vec3& pos) const
    {
        // h < e indicates that we are underground
        float e = 1.0f+currentElevation(pos);
        float h = glm::length(convertToLocal(pos));
        return (h-e);
    }
    float currentGlobalHeight(const glm::vec3& pos) const
    {
        return currentLocalHeight(pos)*scale;
    }
    glm::vec3 currentGroundPos(const glm::vec3& pos, float bias) const
    {
        return glm::vec3(getModelMatrix()*glm::vec4(convertToLocal(pos)*(1.0f-currentLocalHeight(pos)+bias),1.0f));
    }
    void self_spin()
    {
        if(spinning)
            rotation.y += spin_vel*0.02f;
    }
    void gui_interface()
    {
        //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode("Geocube::Control Panel"))
        {
            ImGui::Text("Controllable parameters for Geocube class.");

            // Transform
            ImGui::DragFloat3("Position",&(this->position)[0],0.1f);
            ImGui::DragFloat3("Rotation",&(this->rotation)[0],1.0f);
            ImGui::DragFloat("Scale",&(this->scale),0.01f,0.001f,1e6f);

            ImGui::Checkbox("Self-spin",&spinning);
            if(spinning)
            {
                ImGui::DragFloat("self-spin velocity",&spin_vel,0.001f,0.01f,0.3f);
            }

            // Global transformation
            //ImGui::DragFloat4("rotation", (float*)&rotation,0.01f);
            //ImGui::DragFloat4("refQuaternion", (float*)&refQuaternion,0.01f);

            // Info
            //ImGui::Text("Front  \t%02.6f, %02.6f, %02.6f"  , Front.x,Front.y,Front.z);
            //ImGui::Text("Up     \t%02.6f, %02.6f, %02.6f"     , Up.x,Up.y,Up.z);
            //ImGui::Text("Right  \t%02.6f, %02.6f, %02.6f"  , Right.x,Right.y,Right.z);
            //ImGui::Text("WorldUp\t%02.6f, %02.6f, %02.6f", WorldUp.x,WorldUp.y,WorldUp.z);

            ImGui::TreePop();
        }

    }
protected:
    glm::mat4 getModelMatrix() const
    {
        return glm::translate(glm::scale(glm::mat4(1),glm::vec3(scale)),position)
                *glm::mat4(glm::quat(glm::radians(rotation)));
    }
    glm::vec3 convertToLocal(const glm::vec3& pos) const
    {
        return glm::vec3(glm::inverse(getModelMatrix())*glm::vec4(pos,1.0f));
    }

};


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
    Shader shader(FP("renderer/terrain.vert"),FP("renderer/terrain.frag"));
    Shader lodShader(FP("renderer/lod.vert"),FP("renderer/lod.frag"));
    Shader normalShader(FP("renderer/lod.vert"),FP("renderer/lod.pcolor.frag"),FP("renderer/lod.glsl"));
    Shader lightingShader(FP("renderer/terrain.vert"),FP("renderer/terrain.lighting.frag"));

    // Initlize geogrid system
    Node::init();

    // Initialize lighting system
    Lighting::init();

    // For automatic file reloading
    //FileSystemMonitor::Init(SRC_PATH);

    // Gui interface
    GuiInterface::Init(window);

    // reference camera
    refCamera::shader.reload_shader_program_from_files(FP("renderer/box.vert"),FP("renderer/box.frag"));
    refCamera refcam(camera);

    // lighting
    Lighting dirlight;

    // colormap
    Colormap::Rainbow();

    // read debug texture
    unsigned int debug_tex = loadTexture("texture_debug.jpeg",FP("../../resources/textures"), false);

    // gen geocube
    Geocube mesh;

    while( !glfwWindowShouldClose( window ) )
    {
        // per-frame time logic
        // --------------------
        countAndDisplayFps(window);

        // input
        glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
        processInput(window);

        if(bindCam)
        {
            //float min_height = mesh.currentElevation(refcam.Position) + camera.Near;
            if(mesh.currentLocalHeight(camera.Position) < 5e-7)
                camera.Position = glm::mix(camera.Position, mesh.currentGroundPos(camera.Position, 5e-7), 0.5f);

            //camera.setReference(glm::vec3(0,1,0));
            refcam.sync_frustrum();
            refcam.sync_position();
            refcam.sync_rotation();
        }

        // update geomesh
        mesh.update(refcam);

        // Draw points
        glViewport(0,0,SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // pass matrixes in global coordinate system
        lightingShader.use();
        lightingShader.setVec3("viewPos", camera.Position);

        // Colormap
        Colormap::Bind();
        lightingShader.setInt("colormap", 10);

        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, debug_tex);
        lightingShader.setInt("debugmap", 11);

        // lighting
        dirlight.setParam(lightingShader);

        // render type
        lightingShader.setInt("render_type", Geomesh::RENDER_MODE);

        // todo: remap z buffer to increase valid bit
        // "near" terrains
        glDepthRange(0,0.4);
        camera.Near = 0.002e-4;
        camera.Far = camera.Near*1e4;
        //refcam.sync_frustrum();
        lightingShader.setMat4("projection_view", camera.GetPerspectiveMatrix()*camera.GetViewMatrixOriginBased());
        glEnable(GL_CULL_FACE);
        mesh.draw(lightingShader, refcam);
        glDisable(GL_CULL_FACE);


        // "far" terrains
        glDepthRange(0.4,0.8);
        camera.Near = 100.0e-5;
        camera.Far = camera.Near*1e5;
        //refcam.sync_frustrum();
        lightingShader.setMat4("projection_view", camera.GetPerspectiveMatrix()*camera.GetViewMatrixOriginBased());
        glEnable(GL_CULL_FACE);
        mesh.draw(lightingShader, refcam);
        glDisable(GL_CULL_FACE);

        // "distant" objects
        glDepthRange(0.8,1.0);


        if(drawWireframe)
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //lodShader.use();
            //lodShader.setMat4("projection_view", camera.GetFrustumMatrix());
            //lodShader.setVec3("viewPos", refcam.Position);
            //lodShader.setInt("heightmap", 0);
            glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
            mesh.draw(lightingShader, refcam);
            glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
        }

        if(drawNormalArrows)
        {
            normalShader.use();
            normalShader.setMat4("projection_view", camera.GetFrustumMatrix());
            normalShader.setVec3("viewPos", refcam.Position);
            normalShader.setInt("heightmap", 0);
            normalShader.setVec3("color", glm::vec3(1,1,0));
            mesh.draw(normalShader, refcam);
        }

        // gui
        GuiInterface::Begin();
        mesh.gui_interface();
        Node::gui_interface();
        Geomesh::gui_interface();
        refcam.gui_interface();
        dirlight.gui_interface(camera);
        gui_interface(mesh.currentGlobalHeight(refcam.Position)*6371000);
        ImGui::ShowDemoWindow();
        GuiInterface::End();



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Initlize geogrid system
    Node::finalize();
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
    glfwSwapInterval(0); // 60 fps constraint

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
