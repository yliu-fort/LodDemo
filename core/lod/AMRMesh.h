#ifndef AMRMesh_H
#define AMRMesh_H

#include "PNode.h"
#include <memory>
#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/intersect.hpp"

#include "shader.h"

enum RenderMode
{
    REAL,
    CONTOUR,
    NORMAL,
    PCOLOR,
    ELEMENT_COUNT
};

class AMRMesh : protected AMRNode
{
    // start from the deepest level (leaf node), compute the distance to reference point/camera
    // child sharing the same father are expected to be clustered
    // if all 4 childs are unused -> return the texture handle and merge
    // if node requires further subdivision -> redirect/allocate new texture handle
    // 2 texture handles are required -> appearance (128x128) and height map (17x17)

    //std::shared_ptr<AMRNode> this;
    glm::mat4 global_model_matrix_; // todo: switch to transform

public:
    AMRMesh(glm::mat4 global_model_matrix = glm::mat4(1))
        : AMRNode()
        , global_model_matrix_(global_model_matrix)
    {
        parent_ = static_cast<AMRNode *>(this);
        SetModelMatrix(global_model_matrix_);
        AssignField();
    }

    ~AMRMesh(){}
    AMRMesh(const AMRMesh&) = delete;

    PNode* GetHandle() const { return this->parent_; }


    void Subdivision(uint, uint);
    void Subdivision( const glm::vec3&, const float&, PNode* );


    //void ReleaseAllTextureHandles();
    //void ReleaseAllTextureHandles( AMRNode* node );

    // Caution: only return subdivided grids.
    // write additional condition if you need this
    PNode* QueryNode( const glm::vec2& ) const;

    void Draw( Shader& shader ) const;
    void Draw( const PNode*, Shader& ) const;


    // static functions
    static void GuiInterface();

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
