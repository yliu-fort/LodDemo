#include <iostream>
#include <cmath>
#include <memory>

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

#define VISUAL

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


class refCamera : public Camera
{
public:
    unsigned int cubeVAO=0, cubeVBO=0;
    Camera& reference;
    refCamera(Camera& ref_cam) : Camera(ref_cam), reference(ref_cam) {}
    ~refCamera(){glDeleteVertexArrays(1,&cubeVAO); glDeleteBuffers(1,&cubeVBO); }
    // sync to reference camera's status
    Camera& get_reference()
    {
        return this->reference;
    }
    void set_reference(Camera& cam)
    {
        reference = cam;
    }
    void sync_rotation()
    {
        Front = reference.Front;
        Up = reference.Up;
        Right = reference.Right;
        rotation = reference.rotation;
    }
    void sync_position()
    {
        Position = reference.Position;
    }
    void sync_frustrum()
    {
        Aspect = reference.Aspect;
        Near = reference.Near;
        Far = reference.Far;

        Zoom = reference.Zoom;
    }
    void render_frustrum()
    {

        glm::vec2 s(tanf(0.5f*glm::radians(Zoom)) * Aspect,
                    tanf(0.5f*glm::radians(Zoom)));


        float vertices[] = {
            // x-dir
            -1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            -1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right

            -1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            -1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            // y dir
            -1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right

            1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
            1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
            // z dir
            1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f*s.x * Far,  -1.0f* s.y * Far,  1.0f * Far,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f*s.x * Near, -1.0f* s.y * Near, 1.0f * Near,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right

            -1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,  // bottom-left
            -1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f*s.x * Far,   1.0f* s.y * Far,  1.0f * Far,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            1.0f*s.x * Near,  1.0f* s.y * Near, 1.0f * Near,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right

            // Coordinate rays
            0,0,0,0,0,0,0,0,
            1,0,0,0,0,0,0,0,

            0,0,0,0,0,0,0,0,
            0,1,0,0,0,0,0,0,

            0,0,0,0,0,0,0,0,
            0,0,1,0,0,0,0,0
        };

        // initialize (if necessary)
        if (cubeVAO == 0)
        {
            glGenVertexArrays(1, &cubeVAO);
            glGenBuffers(1, &cubeVBO);
            // fill buffer
            glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STATIC_DRAW);
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
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_LINES, 0, sizeof(vertices)/sizeof(float));
        glBindVertexArray(0);
    }
    void draw_frustrum(Shader& shader)
    {
        glm::mat4 model(1);
        // translate, then rotate locally
        model = glm::translate(model, Position) * glm::mat4(rotation);

        // Set a readable cliping range
        //Near = 0.1f; Far = 10.0f;

        // Draw frustrum
        shader.use();
        shader.setMat4("projection_view", reference.GetFrustumMatrix());
        shader.setMat4("model", model );
        render_frustrum();
    }

};

// renderGrid() renders a 16x16 2d grid in NDC.
// -------------------------------------------------
static unsigned int gridVAO = 0;
static unsigned int gridVBO = 0;
#define GRIDX (24)
#define GRIDY (24)
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

#define HEIGHT_MAP_X (GRIDX + 1)
#define HEIGHT_MAP_Y (GRIDY + 1)
static Shader upsampling, crackfixing;
static unsigned int noiseTex, elevationTex;
struct Node
{
    Node* child[4];
    Node* parent;
    uint appearance, heightmap;
    glm::vec2 lo, hi;
    bool subdivided;
    bool crackfixed;
    uint level, offset_type;
    float elevation;

    Node() : lo(0), hi(1), subdivided(false), crackfixed(false), level(0), offset_type(0),elevation(0)
    {
        glGenTextures(1, &heightmap);
        glGenTextures(1, &appearance);
        //std::cout << "Create node object" << std::endl;
    }
    ~Node()
    {
        if(subdivided)
        {
            delete child[0];
            delete child[1];
            delete child[2];
            delete child[3];
        }

        glDeleteTextures(1, &heightmap);
        glDeleteTextures(1, &appearance);
        //std::cout << "Delete node object" << std::endl;
    }

    template<uint TYPE>
    void setconnectivity(Node* leaf)
    {
        static_assert (TYPE < 4, "Only 4 child index" );
        offset_type = TYPE;
        glm::vec2 center = glm::vec2(0.5f)*(leaf->lo + leaf->hi);
        if(TYPE == 0) // bottom-left
        {
            lo = leaf->lo;
            hi = center;
        }else
        if(TYPE == 1) // top-left
        {
            lo = glm::vec2(leaf->lo.x, center.y);
            hi = glm::vec2(center.x, leaf->hi.y);
        }else
        if(TYPE == 2) // top-right
        {
            lo = center;
            hi = leaf->hi;
        }else
        if(TYPE == 3) // bottom-right
        {
            lo = glm::vec2(center.x, leaf->lo.y);
            hi = glm::vec2(leaf->hi.x, center.y);
        }

        parent = leaf;

        level = parent->level+1;

    }
    glm::vec3 getscale() const
    {
        return glm::vec3(hi.x-lo.x,1.0f,hi.y-lo.y);
    }
    float size() const
    {
        return 0.5f*(hi.x-lo.x + hi.y-lo.y);
    }
    glm::vec3 get_shift() const
    {
        return glm::vec3(lo.x,0.0f,lo.y);
    }
    glm::vec2 get_center() const
    {
        return 0.5f*glm::vec2(hi.x + lo.x,hi.y + lo.y);
    }
    glm::vec3 get_center3flat() const
    {
        return glm::vec3(0.5f*(hi.x + lo.x),0.0f,0.5f*(hi.y + lo.y));
    }
    glm::vec3 get_center3() const
    {
        return glm::vec3(0.5f*(hi.x + lo.x),elevation,0.5f*(hi.y + lo.y));
    }
    void bake_height_map()
    {
        // Initialize 2d heightmap texture

        //Datafield//
        //Store the volume data to polygonise
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, heightmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //Generate a distance field to the center of the cube
        glTexImage2D( GL_TEXTURE_2D, 0, GL_R32F, HEIGHT_MAP_X, HEIGHT_MAP_Y, 0, GL_RED, GL_FLOAT, NULL);

        upsampling.use();
        upsampling.setVec2("lo", lo);
        upsampling.setVec2("hi", hi);

        // bind noisemap
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, noiseTex);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, elevationTex);

        // write to heightmap
        glBindImageTexture(0, heightmap, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

        // Deploy kernel
        glDispatchCompute(1,1,1);

        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Write flag
        crackfixed = false;

    }
    void fix_heightmap(Node* neighbour, int edgedir)
    {
        // edge direction
        // 0: left, 1: top, 2:right, 3:bottom
        glm::vec2 begin,end;
        glm::vec2 my_begin,my_end;
        glm::vec2 texrange;

        //printf(" block range (%f,%f), (%f,%f)\n",lo.x,lo.y, hi.x, hi.y);

        if(edgedir == 0)
        {
            texrange.x = (lo.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y);
            texrange.y = (hi.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y);
            // scale tex.y
            begin = glm::vec2(1, (lo.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y));
            end   = glm::vec2(1, (hi.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y));

            my_begin = glm::vec2(0,0);
            my_end   = glm::vec2(0,1);

            //printf(" trim to left neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
        }
        if(edgedir == 2)
        {
            texrange.x = (lo.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y);
            texrange.y = (hi.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y);
            // scale tex.y
            begin = glm::vec2(0, (lo.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y));
            end   = glm::vec2(0, (hi.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y));

            my_begin = glm::vec2(1,0);
            my_end   = glm::vec2(1,1);

            //printf(" trim to right neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
        }
        if(edgedir == 1)
        {
            texrange.x = (lo.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x);
            texrange.y = (hi.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x);
            // scale tex.x
            begin = glm::vec2((lo.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x), 0);
            end   = glm::vec2((hi.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x), 0);

            my_begin = glm::vec2(0,1);
            my_end   = glm::vec2(1,1);

            //printf(" trim to top neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
        }
        if(edgedir == 3)
        {
            texrange.x = (lo.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x);
            texrange.y = (hi.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x);
            // scale tex.x
            begin = glm::vec2((lo.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x), 1);
            end   = glm::vec2((hi.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x), 1);

            my_begin = glm::vec2(0,0);
            my_end   = glm::vec2(1,0);

            //printf(" trim to bottom neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
        }

        // notice: assumed square height map (hx = hy = 17)
        crackfixing.use();
        crackfixing.setVec2("mylo", my_begin);
        crackfixing.setVec2("myhi", my_end);
        crackfixing.setVec2("shlo", begin);
        crackfixing.setVec2("shhi", end);

        // bind neighbour heightmap
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, neighbour->heightmap);

        // write to heightmap
        glBindImageTexture(0, heightmap, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

        // Deploy kernel
        glDispatchCompute(1,1,1);

        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        // Write flag
        crackfixed = true;
    }
    void split()
    {
        if(!this->subdivided)
        {
            child[0] = new Node;
            child[0]->setconnectivity<0>(this);
            child[0]->bake_height_map();
            child[0]->set_elevation();

            child[1] = new Node;
            child[1]->setconnectivity<1>(this);
            child[1]->bake_height_map();
            child[1]->set_elevation();

            child[2] = new Node;
            child[2]->setconnectivity<2>(this);
            child[2]->bake_height_map();
            child[2]->set_elevation();

            child[3] = new Node;
            child[3]->setconnectivity<3>(this);
            child[3]->bake_height_map();
            child[3]->set_elevation();

            this->subdivided = true;
        }
    }
    int search(glm::vec2 p) const
    {
        glm::vec2 center = get_center();

        // not in box
        if(p.x < lo.x || p.y < lo.y || p.x > hi.x || p.y > hi.y) { return -1; }

        // find in which subregion
        if(p.x < center.x)
        {
            if(p.y < center.y)
                return 0; // bottom-left
            else
                return 1; //top-left
        }else
        {
            if(p.y < center.y)
                return 3; // bottom-right
            else
                return 2; //top-right
        }

    }
    float min_elevation() const
    {
        std::vector<float> heightData(HEIGHT_MAP_X*HEIGHT_MAP_Y);

        // read texture
        glGetTextureImage(heightmap, 0, GL_RED, GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&heightData[0]);

        auto min = std::min(heightData.begin(),heightData.end());
        return *min;
    }
    float get_elevation(const glm::vec2& pos) const
    {
        // pos must located inside this grid
        // use queryGrid() before call this function
        // Caution: getTextureImage is SUPER HEAVY
        // so use this function as few as possible
        // and avoid to call this function every frame
        glm::vec2 relPos = (pos-lo)/(hi-lo);

        uint xoffset = uint(glm::clamp(relPos.x,0.0f,1.0f)*(HEIGHT_MAP_X-1));
        uint yoffset = uint(glm::clamp(relPos.y,0.0f,1.0f)*(HEIGHT_MAP_Y-1));

        float height = 0.0f;
        glGetTextureSubImage(heightmap,
            0,xoffset,yoffset,0,1,1,1,GL_RED,GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&height);

        return height;
    }
    void set_elevation()
    {
        elevation = get_elevation(get_center());
    }
};

#define MAX_DEPTH (9)
class Geomesh
{
public:

    // start from the deepest level (leaf node), compute the distance to reference point/camera
    // child sharing the same father are expected to be clustered
    // if all 4 childs are unused -> return the texture handle and merge
    // if node requires further subdivision -> redirect/allocate new texture handle
    // 2 texture handles are required -> appearance (128x128) and height map (17x17)

    std::shared_ptr<Node> root;
    glm::mat4 model;
    //uint id=0;

    Geomesh() : root(new Node), model(glm::mat4(1)){
        root->bake_height_map();
        root->set_elevation();
    }
    Geomesh(glm::vec2 lo, glm::vec2 hi) : root(new Node){
        root->lo = lo;
        root->hi = hi;
        model = glm::translate(glm::mat4(1),glm::vec3(lo.x,0,lo.y));
        root->bake_height_map();
        root->set_elevation();
    }

    ~Geomesh(){}

    float queryElevation(const glm::vec3& pos) const
    {
        Node* node = queryNode(glm::vec2(pos.x, pos.z));
        if(node)
            return node->get_elevation(glm::vec2(pos.x, pos.z));
        return root->get_elevation(glm::vec2(pos.x, pos.z));
    }

    void refresh_heightmap()
    {
        refresh_heightmap(root.get());
    }

    void fixcrack()
    {
        fixcrack(root.get());
    }

    void subdivision(const glm::vec3& viewPos, const glm::vec3& viewFront)
    {
        subdivision(viewPos, viewFront, root.get());
        refresh_heightmap();
        fixcrack();
    }

    void draw(Shader& shader) const
    {
        drawRecr(root.get(), shader);
    }

    // Caution: only return subdivided grids.
    // write additional condition if you need root
    Node* queryNode( const glm::vec2& pos) const
    {
        //bool isTraversible = node->subdivided;
            // find the node
            Node* sh_node = root.get();
            int result = -1;
            while(true)
            {
                // start from root
                result = sh_node->search(pos);
                if(result != -1 && sh_node->subdivided)
                {
                    sh_node = sh_node->child[result];
                    continue;
                }
                break;
            }

            if(result == -1) { return NULL; }
            return sh_node;
    }

    void refresh_heightmap(Node* node)
    {
        //bool isTraversible = node->subdivided;
        if(node->subdivided)
        {
            refresh_heightmap(node->child[0]);
            refresh_heightmap(node->child[1]);
            refresh_heightmap(node->child[2]);
            refresh_heightmap(node->child[3]);
        }
        else
        {
            // Refresh old height map for interface cells
            if(node->crackfixed)
                node->bake_height_map();
        }
    }

    void fixcrack(Node* node)
    {
        //bool isTraversible = node->subdivided;
        if(node->subdivided)
        {
            fixcrack(node->child[0]);
            fixcrack(node->child[1]);
            fixcrack(node->child[2]);
            fixcrack(node->child[3]);
        }
        else
        {
            // fix crack
            // for each block, we only need to check 2 directions
            // because the other 2 directions are filled by same or higher level lod
            // once found a match, the height map of current block will be modified
            // only perform this to current draw level

            //if(node -> crackfixed == true) { return; }

            // marks: 0: left, 1: top, 2:right, 3:bottom

            glm::vec2 f1, f2;
            int e1, e2;
            if(node->offset_type == 0) // going to search 2 faces
            {
                f1 = node->get_center() - glm::vec2(0.0,(node->hi.y-node->lo.y)); // bottom
                f2 = node->get_center() - glm::vec2((node->hi.x-node->lo.x), 0.0); // left
                e1 = 3;e2 = 0;
            }else
            if(node->offset_type == 1) // going to search 2 faces
            {
                f1 = node->get_center() + glm::vec2(0.0,(node->hi.y-node->lo.y)); // top
                f2 = node->get_center() - glm::vec2((node->hi.x-node->lo.x), 0.0); // left
                e1 = 1;e2 = 0;
            }else
            if(node->offset_type == 2) // going to search 2 faces
            {
                f1 = node->get_center() + glm::vec2(0.0,(node->hi.y-node->lo.y)); // top
                f2 = node->get_center() + glm::vec2((node->hi.x-node->lo.x), 0.0); // right
                e1 = 1;e2 = 2;
            }
            else
            if(node->offset_type == 3) // going to search 2 faces
            {
                f1 = node->get_center() - glm::vec2(0.0,(node->hi.y-node->lo.y)); // bottom
                f2 = node->get_center() + glm::vec2((node->hi.x-node->lo.x), 0.0); // right
                e1 = 3;e2 = 2;
            }
            else {return;}

            // find the node
            Node* sh_node = queryNode(f1);

            // Compare with neighbour, if my_level > neighbour_level
            // a height map sync will be called here
            if(sh_node && node->level == (sh_node->level + 1))
            {
                node-> fix_heightmap(sh_node,e1);
            }

            // here is the second face
            sh_node = queryNode(f2);

            if(sh_node && node->level == (sh_node->level + 1))
            {
                node-> fix_heightmap(sh_node,e2);
            }

            //debug
            //if(node->level == (sh_node->level + 1) || node->level == (sh_node2->level + 1))
            //    {
            //        shader.use();
            //        // Transfer local grid model
            //        glm::mat4 model(1);
            //
            //        model = glm::translate(model, node->getshift());
            //        model = glm::scale(model, node->getscale());
            //        shader.setMat4("model", model);
            //
            //        // Transfer lo and hi
            //        shader.setVec2("lo", node->lo);
            //        shader.setVec2("hi", node->hi);
            //
            //        // Active textures
            //        glActiveTexture(GL_TEXTURE0);
            //        glBindTexture(GL_TEXTURE_2D, node->heightmap);
            //
            //        // Render grid
            //        renderGrid();
            //    }
            //    {
            //printf(" node [(%f,%f),(%f,%f),%d] find two adjcent faces"
            //       " [(%f,%f),(%f,%f),%d] and node [(%f,%f),(%f,%f),%d]\n",
            //       node->lo.x,node->lo.y,node->hi.x,node->hi.y,node->level,
            //       sh_node->lo.x,sh_node->lo.y,sh_node->hi.x,sh_node->hi.y,sh_node->level,
            //       sh_node2->lo.x,sh_node2->lo.y,sh_node2->hi.x,sh_node2->hi.y,sh_node2->level
            //       );
            //    }
            //std::cout <<std::endl;


            //node -> crackfixed = true;
        }
    }

    void subdivision(const glm::vec3& viewPos, const glm::vec3& viewFront, Node* node)
    {

        // distance between nodepos and viewpos
        float d = glm::distance(viewPos, node->get_center3());

        // frustrum culling
        float K = 2.8f*node->size();
        K = glm::dot(viewFront, node->get_center3() - viewPos) > 0.0f || d < node->size() ? K:0.0f;

        // Subdivision
        if( d < K && node->level < MAX_DEPTH )
        {

            // split and bake heightmap
            node->split();

            subdivision(viewPos, viewFront, node->child[0]);
            subdivision(viewPos, viewFront, node->child[1]);
            subdivision(viewPos, viewFront, node->child[2]);
            subdivision(viewPos, viewFront, node->child[3]);

        }
        else
        {
            if(d > (1.15f * K) && node->subdivided)
            {
                delete node->child[0];
                delete node->child[1];
                delete node->child[2];
                delete node->child[3];

                node->subdivided = false;
            }
        }
    }

    void drawRecr(Node* node, Shader& shader) const
    {
        if(node->subdivided)
        {
            drawRecr(node->child[0], shader);
            drawRecr(node->child[1], shader);
            drawRecr(node->child[2], shader);
            drawRecr(node->child[3], shader);
        }
        else
        {
            // Transfer local grid model
            //glm::mat4 model(1);

            glm::mat4 _model = glm::translate(glm::mat4(1), node->get_shift());
            _model = glm::scale(_model, node->getscale());
            shader.setMat4("model", _model);

            // Transfer lo and hi
            //shader.setVec2("lo", node->lo);
            //shader.setVec2("hi", node->hi);

            // Active textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, node->heightmap);

            // Render grid
            renderGrid();
        }
        return;
    }
};

// which mesh I am standing
float currentElevation(std::vector<Geomesh>& mesh, const glm::vec3& pos)
{
    for(auto& land: mesh)
    {
        // Caution: query in 2d grid on (x,z) plane
        if(land.root->lo.x < pos.x && land.root->lo.y < pos.z
        && land.root->hi.x >= pos.x && land.root->hi.y >= pos.z)
        {
            return land.queryElevation(pos);
        }
    }
    return 0.0f;
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

    // Read shader
    Shader shader(FP("renderer/terrain.vert"),FP("renderer/terrain.frag"));
    Shader pColorShader(FP("renderer/box.vert"),FP("renderer/box.frag"));
    Shader lodShader(FP("renderer/lod.vert"),FP("renderer/lod.frag"));
    Shader lightingShader(FP("renderer/terrain.vert"),FP("renderer/terrain.lighting.frag"));

    // For automatic file reloading
    FileSystemMonitor::Init(SRC_PATH);

    // reference camera
    refCamera refcam(camera);

    // Initialize 2d noise texture
    //Datafield//

    //Store the volume data to polygonise
    glGenTextures(1, &noiseTex);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    //Generate a distance field to the center of the cube
    std::vector<float> dataField;
    uint res = 256;
    for(uint j=0; j<res; j++){
        for(uint i=0; i<res; i++){
            dataField.push_back( rand() / double(RAND_MAX) );
            //dataField.push_back(sqrt(i*i + j*j)/double(res));
        }
    }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_R32F, res, res, 0, GL_RED, GL_FLOAT, &dataField[0]);

    // prescribed elevation map
    glGenTextures(1, &elevationTex);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, elevationTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    //Generate a distance field to the center of the cube
    dataField.clear();
    res = 1024;
    for(uint j=0; j<res; j++){
        for(uint i=0; i<res; i++){
            //if(i > res/3)
            //    dataField.push_back( 0.5*(tanh(0.02f*(i - res/3)-1)) );
            //else
            //    dataField.push_back(0);

            dataField.push_back( 0.5f*(tanh(-0.005f*sqrt((i - res/2)*(i - res/2) + (j- res/2)*(j - res/2))+1)) );
        }
    }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_R32F, res, res, 0, GL_RED, GL_FLOAT, &dataField[0]);


    // colormap
    //Colormap::Rainbow();
    Colormap::Rainbow();

    // real material texture
    unsigned int material = loadTexture("Y42lf.png",FP("../../resources/textures"), false);

    // Geo mesh, careful: need a noise texture and shader before intialized
    upsampling.reload_shader_program_from_files(FP("renderer/upsampling.glsl"));
    crackfixing.reload_shader_program_from_files(FP("renderer/crackfixing.glsl"));

    std::vector<Geomesh> mesh;
    for(int i = -8; i < 8; i++)
        for(int j = -8; j < 8; j++)
        {
            Geomesh land(glm::vec2(i,j),glm::vec2(i+1,j+1));
            mesh.push_back(land);
        }

    // Adjust camera frustum
    camera.Near = 100.0/6e6;
    //camera.Far = 2.0;

    while( !glfwWindowShouldClose( window ) )
    {
        // per-frame time logic
        // --------------------
        bool ticking = countAndDisplayFps(window);

#ifdef VISUAL
        // input
        glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
        processInput(window);

        for(auto& land: mesh)
        {
            land.subdivision(refcam.Position, refcam.Front);
        }

        if(bindCam)
        {
            float min_height = currentElevation(mesh, camera.Position) + 10.0f*camera.Near;
            camera.Position.y = fmaxf(camera.Position.y, min_height);
            camera.setReference(glm::vec3(0,1,0));
            refcam.sync_frustrum();
            refcam.sync_position();
            refcam.sync_rotation();

        }

        if(ticking)
        {
            std::cout << "REAL HEIGHT = " << refcam.Position.y*6e3 << "m" << std::endl;
            std::cout << "Elevation = " << currentElevation(mesh, refcam.Position)*6e3 << "m" << std::endl;
        }

        // Draw points
        glViewport(0,0,SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.4f, 0.4f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        // Render relative to eye
        lightingShader.use();
        lightingShader.setMat4("projection_view", camera.GetFrustumMatrix());
        lightingShader.setVec3("viewPos", refcam.Position);

        // Noise texture
        lightingShader.setInt("heightmap", 0);

        // Colormap
        Colormap::Bind();
        lightingShader.setInt("colormap", 10);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, material);
        lightingShader.setInt("material", 2);

        // lighting
        lightingShader.setVec3("dirLight.direction", glm::vec3(12.5f*sinf(0.1f*glfwGetTime()),-4.3f,12.5f*cosf(0.1f*glfwGetTime())));
        lightingShader.setVec3("dirLight.ambient", glm::vec3( 0.05f, 0.05f, 0.05f));
        lightingShader.setVec3("dirLight.diffuse", glm::vec3( 1.5f, 1.5f, 1.5f));
        lightingShader.setVec3("dirLight.specular", glm::vec3( 0.10f, 0.15f, 0.10f));

        //renderPlane();
        //renderGrid();
        //mesh.draw(shader);
        for(const auto& land: mesh)
        {
            land.draw(lightingShader);
        }

        //glClear(GL_COLOR_BUFFER_BIT);
        //
        //lodShader.use();
        //lodShader.setMat4("projection_view", camera.GetFrustumMatrix());
        //lodShader.setVec3("viewPos", refcam.Position);
        //lodShader.setInt("heightmap", 0);
        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        //for(auto& land: mesh) { land.draw(lodShader); }
        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

        // debug
        //refcam.draw_frustrum(pColorShader);

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
