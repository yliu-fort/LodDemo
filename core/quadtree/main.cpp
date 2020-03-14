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
//#include "colormap.h"
#include "filesystemmonitor.h"
//#include "gui_interface.h"
//#include "slice.h"
//#include "boundingbox.h"
#include "image_io.h"
//#include "cartesian_field.h"

// settings
static int SCR_WIDTH  = 800;
static int SCR_HEIGHT = 600;

// camera
static Camera camera = Camera(glm::vec3(0.0f, 1.0f, 1.0f), float(SCR_WIDTH)/SCR_HEIGHT);

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

// renderCube() renders a 16x16 2d grid in NDC.
// -------------------------------------------------
static unsigned int gridVAO = 0;
static unsigned int gridVBO = 0;
void renderGrid()
{
    int nx=16,ny=16;

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

        for(int j = 0; j < ny; j++)
            for(int i = 0; i < nx; i++)
            {
                glm::vec2 lo(    i/float(nx),    j/float(ny));
                glm::vec2 hi((i+1)/float(nx),(j+1)/float(ny));

                vertices.push_back(float8( lo.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, lo.x, lo.y)); // bottom-left
                vertices.push_back(float8( lo.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, lo.x, hi.y)); // top-left
                vertices.push_back(float8( hi.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, hi.x, hi.y)); // top-right

                vertices.push_back(float8( hi.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, hi.x, hi.y)); // top-right
                vertices.push_back(float8( hi.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, hi.x, lo.y)); // bottom-right
                vertices.push_back(float8( lo.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, lo.x, lo.y)); // bottom-left

            }

        //float vertices[] = {
        //    // z dir
        //     1.0f, 0.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        //     1.0f, 0.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
        //    -1.0f, 0.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        //    -1.0f, 0.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        //
        //};

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
    // render Cube
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6*nx*ny);
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
        Near = 0.1f; Far = 10.0f;

        // Draw frustrum
        shader.use();
        shader.setMat4("projection_view", reference.GetFrustumMatrix());
        shader.setMat4("model", model );
        render_frustrum();
    }

};

struct Node
{
    Node* child[4];
    Node* parent;
    uint appearance, heightmap;
    glm::vec2 lo, hi;
    bool subdivided = false;
    uint level = 0;

    Node() : lo(0), hi(1) { }
    ~Node()
    {
        if(subdivided)
        {
            delete child[0];
            delete child[1];
            delete child[2];
            delete child[3];
        }
    }

    template<uint TYPE>
    void setconnectivity(Node* leaf)
    {
        static_assert (TYPE < 4, "Only 4 child index" );
        glm::vec2 center = glm::vec2(0.5f)*(leaf->lo + leaf->hi);
        if(TYPE == 0) // bottom-left
        {
            lo = leaf->lo;
            hi = center;
        }
        if(TYPE == 1) // top-left
        {
            lo = glm::vec2(leaf->lo.x, center.y);
            hi = glm::vec2(center.x, leaf->hi.y);
        }
        if(TYPE == 2) // top-right
        {
            lo = center;
            hi = leaf->hi;
        }
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
    glm::vec3 getshift() const
    {
        return glm::vec3(lo.x,0.0f,lo.y);
    }
    glm::vec3 getcenter() const
    {
        return 0.5f*glm::vec3(hi.x + lo.x,0.0f,hi.y + lo.y);
    }
};

class Geomesh
{
public:
    glm::mat4 model;
    unsigned int VAO, VBO, EBO;

    // start from the deepest level (leaf node), compute the distance to reference point/camera
    // child sharing the same father are expected to be clustered
    // if all 4 childs are unused -> return the texture handle and merge
    // if node requires further subdivision -> redirect/allocate new texture handle
    // 2 texture handles are required -> appearance (128x128) and height map (16x16)

    Node* root;

    Geomesh(){ root = new Node; }
    ~Geomesh(){ delete root; }

    void subdivision(glm::vec3 viewPos)
    {
        subdivision(viewPos, root);
    }
    void subdivision(glm::vec3 viewPos, Node* node)
    {

        float d = glm::distance(viewPos, node->getcenter());
        float K = 2.8f;

#define MAX_DEPTH (9)

        // Subdivision
        if(d < (K * node->size()) && node->level < MAX_DEPTH)
        {

            if(!node->subdivided)
            {
                node->child[0] = new Node;
                node->child[0]->setconnectivity<0>(node);

                node->child[1] = new Node;
                node->child[1]->setconnectivity<1>(node);

                node->child[2] = new Node;
                node->child[2]->setconnectivity<2>(node);

                node->child[3] = new Node;
                node->child[3]->setconnectivity<3>(node);

                node->subdivided = true;
            }

            subdivision(viewPos, node->child[0]);
            subdivision(viewPos, node->child[1]);
            subdivision(viewPos, node->child[2]);
            subdivision(viewPos, node->child[3]);

        }
        else
        {
            if(d > (1.15f* K * node->size()) && node->subdivided)
            {
                delete node->child[0];
                delete node->child[1];
                delete node->child[2];
                delete node->child[3];

                node->subdivided = false;
            }
        }
    }

    void draw(Shader& shader)
    {
        drawRecr(root, shader);
    }
    void drawRecr(Node* node, Shader& shader)
    {
        bool isTraversible = node->subdivided;
        if(isTraversible)
        {
            drawRecr(node->child[0], shader);
            drawRecr(node->child[1], shader);
            drawRecr(node->child[2], shader);
            drawRecr(node->child[3], shader);
        }
        else
        {
            glm::mat4 model(1);

            model = glm::translate(model, node->getshift());
            model = glm::scale(model, node->getscale());
            shader.setMat4("model", model);
            shader.setVec4("lod_level", glm::vec4(node->lo.x,0,node->lo.y,node->level));

            renderGrid();
        }
        return;
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
    Shader pColorShader(FP("renderer/box.vert"),FP("renderer/box.frag"));
    Shader lodShader(FP("renderer/lod.vert"),FP("renderer/lod.frag"));

    // For automatic file reloading
    FileSystemMonitor::Init(SRC_PATH);

    // Initialize 2d noise texture
    uint nx = 256, ny = 256;
    //Datafield//
    unsigned int dataFieldTex;
    //Store the volume data to polygonise
    glGenTextures(1, &dataFieldTex);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, dataFieldTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //Generate a distance field to the center of the cube
    std::vector<glm::vec4> dataField;
    for(uint j=0; j<ny; j++)
        for(uint i=0; i<nx; i++){
            dataField.push_back(glm::vec4(rand() / double(RAND_MAX),
                                          rand() / double(RAND_MAX),
                                          rand() / double(RAND_MAX),1.0f));
            //dataField.push_back(glm::vec4(i/(float)nx,
            //                              j/(float)ny,
            //                              0.0f,1.0f));
        }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, nx, ny, 0, GL_RGBA, GL_FLOAT, &dataField[0]);


    // reference camera
    refCamera refcam(camera);

    // Geo mesh
    Geomesh mesh;

    while( !glfwWindowShouldClose( window ) )
    {
        // per-frame time logic
        // --------------------
        bool ticking = countAndDisplayFps(window);

#ifdef VISUAL
        // input
        glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
        processInput(window);

        if(bindCam)
        {
            refcam.sync_frustrum();
            refcam.sync_position();
            refcam.sync_rotation();
        }

        mesh.subdivision(refcam.Position);

        // Draw points
        glViewport(0,0,SCR_WIDTH, SCR_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        // Render relative to eye
        shader.use();
        shader.setMat4("projection_view", camera.GetFrustumMatrix());
        shader.setMat4("model", glm::mat4(1));
        shader.setVec3("viewPos", refcam.Position);

        shader.setInt("tex0", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dataFieldTex);

        //renderPlane();
        //renderGrid();
        mesh.draw(shader);

        //lodShader.use();
        //lodShader.setMat4("projection_view", camera.GetFrustumMatrix());
        //lodShader.setMat4("model", glm::mat4(1));
        //lodShader.setVec3("viewPos", refcam.Position);
        //
        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        //mesh.draw(lodShader);
        //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

        // debug
        refcam.draw_frustrum(pColorShader);


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
