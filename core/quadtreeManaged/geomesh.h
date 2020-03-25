#ifndef GEOMESH_H
#define GROMESH_H

#include "grid.h"
#include <memory>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "shader.h"

enum RenderMode
{
    REAL,
    CONTOUR,
    NORMAL,
    PCOLOR,
    ELEMENT_COUNT
};

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
        if(CRACK_FILLING){fixcrack();}
    }

    void draw(Shader& shader) const
    {
        drawRecr(root.get(), shader);
    }

    // Caution: only return subdivided grids.
    // write additional condition if you need root
    Node* queryNode( const glm::vec2& ) const;
    void refresh_heightmap( Node* );
    void fixcrack( Node* );
    void subdivision( const glm::vec3&, const glm::vec3&, Node* );
    void drawRecr( Node*, Shader& ) const;

    // static functions
    static void gui_interface();

//protected:
    // static member
    static uint MAX_DEPTH;
    static float CUTIN_FACTOR;
    static float CUTOUT_FACTOR;
    static bool FRUSTRUM_CULLING;
    static bool CRACK_FILLING;
    static RenderMode RENDER_MODE;
};

#endif
