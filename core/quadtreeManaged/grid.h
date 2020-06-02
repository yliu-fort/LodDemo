#ifndef GRID2D_H
#define GRID2D_H
#include <iostream>
#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

typedef unsigned int uint;

// sizes
#define GRIDX (16)
#define GRIDY (16)
#define HEIGHT_MAP_X (GRIDX+1)
#define HEIGHT_MAP_Y (GRIDY+1)

// Node class
class Node
{
public:
    Node* child[4];
    Node* parent;
    uint appearance, heightmap;

    glm::vec2 lo, hi; // global coordinates
    glm::vec2 rlo, rhi; // relative coordinates
    bool subdivided;
    bool crackfixed;
    uint level, offset_type;
    float elevation;

    glm::mat4 model;

    Node();
    ~Node();

    void set_model_matrix()
    {
        model = glm::translate(glm::mat4(1), this->get_shift());
        model = glm::scale(model, this->get_scale());
    }
    float size() const
    {
        return 0.5f*(hi.x-lo.x + hi.y-lo.y);
    }
    glm::mat4 get_model_matrix() const
    {
        return this->model;
    }
    glm::vec3 get_scale() const
    {
        return glm::vec3(hi.x-lo.x,1.0f,hi.y-lo.y);
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
    void bake_height_map();
    void fix_heightmap(Node* neighbour, int edgedir);
    void split();
    int search(glm::vec2 p) const;

    float min_elevation() const;
    float get_elevation(const glm::vec2& pos) const;
    void set_elevation();

    template<uint TYPE>
    void setconnectivity(Node* leaf);

    // Static function
    static void init();
    static void finalize();
    static void draw();
    static void gui_interface();

    // static member
    static uint NODE_COUNT;
    static uint INTERFACE_NODE_COUNT;
    static bool USE_CACHE;
};

#endif
