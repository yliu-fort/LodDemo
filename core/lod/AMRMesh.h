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
    std::vector<std::vector<PNode*>> level_order_list_;
    bool modified_;

public:
    AMRMesh(glm::mat4 global_model_matrix)
        : AMRNode()
        , global_model_matrix_(global_model_matrix)
        ,level_order_list_()
        ,modified_(true)
    {
        parent_ = static_cast<AMRNode *>(this);
        SetModelMatrix(global_model_matrix_);
        AssignField();
        UpdateLevelOrderList();
    }

    ~AMRMesh(){}
    AMRMesh(const AMRMesh&) = delete;

    PNode* GetHandle() const { return this->parent_; }

    //void ReleaseAllTextureHandles();
    //void ReleaseAllTextureHandles( AMRNode* node );

    // Caution: only return subdivided grids.
    // write additional condition if you need this
    PNode* QueryNode( const glm::vec2& ) const;
    PNode* QueryNode( uint ) const;
    void Subdivision(uint, uint);
    void CircularSubdivision(uint, uint);
    void Draw( Shader& shader ) const;
    void MultiLevelIntegrator();
    void UpdateLevelOrderList()
    {
        if(modified_){
            level_order_list_ = GetLevelOrder(GetHandle());
            std::cout << "level-order list updated.\n";
        }
        modified_ = false;
    }

    // static functions
    static void GuiInterface();

    //protected:
    // static member
    static uint MIN_DEPTH, MAX_DEPTH;
    static float CUTIN_FACTOR;
    static float CUTOUT_FACTOR;
    static RenderMode RENDER_MODE;
};

#endif
