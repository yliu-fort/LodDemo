#ifndef OGeomesh_H
#define OGeomesh_H

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

class OGeomesh : protected PGeoNode
{
    // start from the deepest level (leaf node), compute the distance to reference point/camera
    // child sharing the same father are expected to be clustered
    // if all 4 childs are unused -> return the texture handle and merge
    // if node requires further subdivision -> redirect/allocate new texture handle
    // 2 texture handles are required -> appearance (128x128) and height map (17x17)

    //std::shared_ptr<PNode> this;
    glm::mat4 global_model_matrix_; // todo: switch to transform

public:
    OGeomesh(glm::mat4 global_model_matrix = glm::mat4(1))
        : PGeoNode()
        , global_model_matrix_(global_model_matrix)
    {
        parent_ = static_cast<PGeoNode *>(this);
        SetModelMatrix(global_model_matrix_);
        BakeHeightMap(global_model_matrix_);
        BakeAppearanceMap(global_model_matrix_);
        SetElevation();
    }

    ~OGeomesh(){}
    OGeomesh(const OGeomesh&) = delete;

    PNode* GetHandle() const { return static_cast<PGeoNode *>(this->parent_); }

    glm::vec3 ConvertToDeformed(const glm::vec3& v) const;
    glm::vec3 ConvertToNormal(const glm::vec3& v) const;
    glm::vec3 ConvertToUV(const glm::vec3& v) const;
    bool IsGroundReference(const glm::vec3& pos) const;
    float QueryElevation(const glm::vec3& pos) const;
    void Subdivision(const glm::vec3& viewPos);
    void Subdivision(uint level);
    void Subdivision(uint, uint);
    void Draw(Shader& shader, const glm::vec3& viewPos) const;
    void ReleaseAllTextureHandles();
    void ReleaseAllTextureHandles( PGeoNode* node );

    // Caution: only return subdivided grids.
    // write additional condition if you need this
    PNode* QueryNode( const glm::vec2& ) const;
    void RefreshHeightmap( PGeoNode* );
    void Fixcrack( PGeoNode* );
    void Subdivision( const glm::vec3&, const float&, PGeoNode* );
    void Subdivision( uint, PGeoNode* );
    void Draw( const PGeoNode*, Shader& ) const;


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
