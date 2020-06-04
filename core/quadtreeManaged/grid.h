#ifndef GRID2D_H
#define GRID2D_H
#include <iostream>
#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <memory>
#include <tuple>
typedef unsigned int uint;

// sizes
#define GRIDX (18)
#define GRIDY (18)
#define HEIGHT_MAP_X (GRIDX+1)
#define HEIGHT_MAP_Y (GRIDY+1)
#define ALBEDO_MAP_X (127)
#define ALBEDO_MAP_Y (127)

// Node class
class Node
{
public:
    Node* child[4];
    Node* parent;
    uint heightmap;
    uint appearance;

    glm::vec2 lo, hi; // global coordinates
    glm::vec2 rlo, rhi; // relative coordinates
    bool subdivided;
    bool crackfixed;
    uint level, offset_type;
    float elevation;

    glm::mat4 model;

    Node();
    ~Node();

    void set_model_matrix(const glm::mat4& arg)
    {
        model = glm::translate(arg, this->get_shift());
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
    glm::vec3 get_relative_shift() const
    {
        return glm::vec3(rlo.x,0.0f,rlo.y);
    }
    glm::vec3 get_relative_center() const
    {
        return glm::vec3(0.5f*(rhi.x + rlo.x),0.0f,0.5f*(rhi.y + rlo.y));
    }
    void bake_height_map(glm::mat4 arg);
    void bake_appearance_map(glm::mat4 arg);
    void fix_heightmap(Node* neighbour, int edgedir);
    void split(glm::mat4 arg);
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
    static std::vector<std::tuple<uint,uint>> CACHE;
};

#endif
