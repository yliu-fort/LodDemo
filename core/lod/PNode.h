#ifndef PNODE_H
#define PNODE_H
#include <iostream>
#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <memory>
#include <tuple>

typedef unsigned int uint;
class Shader;

// Node class
// implemented basic tree operation
class PNode
{
protected:
    static unsigned int gridVAO;
    static unsigned int gridVBO;
    static void RenderGrid();

protected:
    bool subdivided_;

    PNode();
    virtual ~PNode();
    PNode(const PNode&) = delete;

public:
    glm::mat4 model_;

    PNode* child_[4];
    PNode* parent_;

    uint level_ = 0;
    int morton_ = 0x2; // non-zero bit for level detection

    glm::vec2 lo_, hi_; // global coordinates
    glm::vec2 rlo_, rhi_; // relative coordinates

    bool IsSubdivided() const
    {
        return subdivided_;
    }

    void SetModelMatrix(const glm::mat4& arg)
    {
        model_ = glm::translate(arg, this->GetShift());
        model_ = glm::scale(model_, this->GetScale());
    }
    float Size() const
    {
        return 0.5f*(hi_.x-lo_.x + hi_.y-lo_.y);
    }
    glm::mat4 GetModelMatrix() const
    {
        return this->model_;
    }
    glm::vec3 GetScale() const
    {
        return glm::vec3(hi_.x-lo_.x,1.0f,hi_.y-lo_.y);
    }
    glm::vec3 GetShift() const
    {
        return glm::vec3(lo_.x,0.0f,lo_.y);
    }
    glm::vec2 GetCenter() const
    {
        return 0.5f*glm::vec2(hi_.x + lo_.x,hi_.y + lo_.y);
    }
    glm::vec3 GetCenter3flat() const
    {
        return glm::vec3(0.5f*(hi_.x + lo_.x),0.0f,0.5f*(hi_.y + lo_.y));
    }
    //glm::vec3 GetCenter3() const
    //{
    //    return glm::vec3(0.5f*(hi_.x + lo_.x),elevation_,0.5f*(hi_.y + lo_.y));
    //}
    glm::vec3 GetRelativeShift() const
    {
        return glm::vec3(rlo_.x,0.0f,rlo_.y);
    }
    glm::vec3 GetRelativeCenter() const
    {
        return glm::vec3(0.5f*(rhi_.x + rlo_.x),0.0f,0.5f*(rhi_.y + rlo_.y));
    }
    uint GetOffsetType() const
    {
        if(level_ == 0) return -1;
        return morton_&0x3;
    }
    virtual void Split(const glm::mat4& arg);
    int Search(glm::vec2 p) const;

    template<uint TYPE>
    void SetConnectivity(PNode* leaf);

    void ReleaseConnectedNodes()
    {
        if(subdivided_)
        {
            delete child_[0];
            delete child_[1];
            delete child_[2];
            delete child_[3];
        }
        subdivided_ = false;
    }

    // static function
    static void Draw();

    // static member
    static uint NODE_COUNT;
    static uint INTERFACE_NODE_COUNT;

    static constexpr int GRIDX =(18);
    static constexpr int GRIDY =(18);

};

class PGeoNode : public PNode
{
protected:
    bool crackfixed_ = false;
    bool texture_handle_allocated_ = false;
    float elevation_ = 0;

    PGeoNode();
    virtual ~PGeoNode();
    PGeoNode(const PGeoNode&) = delete;

public:
    uint heightmap_;
    uint appearance_;
    uint normal_;

    void BakeHeightMap(const glm::mat4& arg);
    void BakeAppearanceMap(const glm::mat4& arg);
    void FixHeightMap(PNode* neighbour, int edgedir);
    virtual void Split(const glm::mat4& arg) override;

    float MinElevation() const;
    float GetElevation(const glm::vec2& pos) const;
    void SetElevation();

    void QueryTextureHandle();
    void ReleaseTextureHandle();

    // Static function
    static void Init();
    static void Finalize();
    static void GuiInterface();

    // Static member
    static bool USE_CACHE;
    static std::vector<std::tuple<uint,uint,uint>> CACHE;

    static constexpr int HEIGHT_MAP_X =(PNode::GRIDX+1);
    static constexpr int HEIGHT_MAP_Y =(PNode::GRIDY+1);
    static constexpr int ALBEDO_MAP_X =(127);
    static constexpr int ALBEDO_MAP_Y =(127);

    static constexpr auto HEIGHT_MAP_INTERNAL_FORMAT = GL_RGBA32F;
    static constexpr auto HEIGHT_MAP_FORMAT = GL_RGBA;
    static constexpr auto APPEARANCE_MAP_INTERNAL_FORMAT = GL_RGBA8;
    static constexpr uint MAX_CACHE_CAPACITY = 1524;

    static Shader upsampling, crackfixing, appearance_baking;
    static unsigned int noiseTex, elevationTex, materialTex;

};
#endif
