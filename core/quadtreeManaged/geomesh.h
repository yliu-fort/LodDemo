#ifndef GEOMESH_H
#define GROMESH_H

#include "grid.h"
#include <memory>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/intersect.hpp"

#include "shader.h"
#include <glad/glad.h>

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
    // start from the deepest level (leaf node), compute the distance to reference point/camera
    // child sharing the same father are expected to be clustered
    // if all 4 childs are unused -> return the texture handle and merge
    // if node requires further subdivision -> redirect/allocate new texture handle
    // 2 texture handles are required -> appearance (128x128) and height map (17x17)

    Node* root;
    glm::mat4 model;

public:

    Geomesh() : root(new Node), model(glm::mat4(1)){
        root->parent = root;
        root->set_model_matrix(model);
        root->bake_height_map(model);
        root->bake_appearance_map(model);
        root->set_elevation();

    }
    Geomesh(glm::mat4 arg) : root(new Node), model(arg){
        //root->lo = lo;
        //root->hi = hi;
        root->parent = root; // should not cause cyclic referencing
        root->set_model_matrix(model);
        root->bake_height_map(model);
        root->bake_appearance_map(model);
        root->set_elevation();
    }

    ~Geomesh(){ delete root; }

    glm::vec3 convertToDeformed(const glm::vec3& v) const
    {
        return glm::vec3(model*glm::vec4(v,1.0));
    }
    glm::vec3 convertToNormal(const glm::vec3& v) const
    {
        return glm::vec3(glm::inverse(model)*glm::vec4(v,1.0));
    }
    glm::vec3 convertFromSphere(const glm::vec3& v) const
    {
        auto nv = glm::normalize(v); // normalize to R=1
        float d;
        auto orig = convertToDeformed(glm::vec3(0));
        auto dir = glm::normalize(convertToDeformed(glm::vec3(0,1,0)));
        bool intersected = glm::intersectRayPlane	(	glm::vec3(0),
                                                        -nv,
                                                        orig,
                                                        dir,
                                                        d);

        if(!intersected)
            return glm::vec3(9999);

        nv = convertToNormal(nv*fabsf(d)); // project to plane then transform to +y plane
        nv.y = glm::length(v) - 1.0f;

        return nv;
    }
    bool isGroundReference(const glm::vec3& pos) const
    {
        auto tpos = convertFromSphere(pos);
        if(abs(tpos.x)<= 1 && abs(tpos.z)<= 1)
            return true;
        return false;
    }

    float queryElevation(const glm::vec3& pos) const
    {
        auto tpos = convertFromSphere(pos);
        Node* node = queryNode(glm::vec2(tpos.x, tpos.z));
        if(node)
            return node->get_elevation(glm::vec2(tpos.x, tpos.z));
        return root->get_elevation(glm::vec2(tpos.x, tpos.z));
    }

    void refresh_heightmap()
    {
        refresh_heightmap(root);
    }

    void fixcrack()
    {
        fixcrack(root);
    }

    void subdivision(const glm::vec3& viewPos, const glm::vec3& viewFront)
    {
        //auto v = convertFromSphere(viewPos);
        //std::cout << v.x << ", " << v.y << ", " << v.z << std::endl;
        subdivision(convertFromSphere(viewPos), viewFront, queryElevation(viewPos), root);
        //refresh_heightmap();
        //if(CRACK_FILLING){fixcrack();}
    }

    void draw(Shader& shader, const glm::vec3& viewPos) const
    {
        shader.setVec3("projPos",convertFromSphere(viewPos));
        drawRecr(root, shader);
    }

    // Caution: only return subdivided grids.
    // write additional condition if you need root
    Node* queryNode( const glm::vec2& ) const;
    void refresh_heightmap( Node* );
    void fixcrack( Node* );
    void subdivision( const glm::vec3&, const glm::vec3&, const float&, Node* );
    void drawRecr( Node*, Shader& ) const;

    // static functions
    static void gui_interface();

    //protected:
    // static member
    static uint MIN_DEPTH, MAX_DEPTH;
    static float CUTIN_FACTOR;
    static float CUTOUT_FACTOR;
    static bool FRUSTRUM_CULLING;
    static bool CRACK_FILLING;
    static RenderMode RENDER_MODE;
};

#endif
