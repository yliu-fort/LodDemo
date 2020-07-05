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

constexpr int GRIDX =(18);
constexpr int GRIDY =(18);
constexpr int HEIGHT_MAP_X =(GRIDX+1);
constexpr int HEIGHT_MAP_Y =(GRIDY+1);
constexpr int ALBEDO_MAP_X =(127);
constexpr int ALBEDO_MAP_Y =(127);

// Node class
class PNode
{

protected:
    bool subdivided_;
    bool crackfixed_;
    bool texture_handle_allocated_ = false;

    float elevation_;

    PNode();
    ~PNode();
    PNode(const PNode&) = delete;

public:

    glm::mat4 model_;

    uint heightmap_;
    uint appearance_, normal_;

    PNode* child_[4];
    PNode* parent_;

    uint level_ = 0, offset_type_;
    int morton_ = 0x2; //for level detection

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
    glm::vec3 GetCenter3() const
    {
        return glm::vec3(0.5f*(hi_.x + lo_.x),elevation_,0.5f*(hi_.y + lo_.y));
    }
    glm::vec3 GetRelativeShift() const
    {
        return glm::vec3(rlo_.x,0.0f,rlo_.y);
    }
    glm::vec3 GetRelativeCenter() const
    {
        return glm::vec3(0.5f*(rhi_.x + rlo_.x),0.0f,0.5f*(rhi_.y + rlo_.y));
    }
    void BakeHeightMap(const glm::mat4& arg);
    void BakeAppearanceMap(const glm::mat4& arg);
    void FixHeightMap(PNode* neighbour, int edgedir);
    void Split(const glm::mat4& arg);
    int Search(glm::vec2 p) const;

    float MinElevation() const;
    float GetElevation(const glm::vec2& pos) const;
    void SetElevation();

    void QueryTextureHandle();
    void ReleaseTextureHandle();

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

    // Static function
    static void Init();
    static void Finalize();
    static void Draw();
    static void GuiInterface();

    // static member
    static uint NODE_COUNT;
    static uint INTERFACE_NODE_COUNT;
    static bool USE_CACHE;
    static std::vector<std::tuple<uint,uint,uint>> CACHE;


};

#endif
