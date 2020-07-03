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
    bool subdivided;
    bool crackfixed;
    bool textureHandleAllocated = false;

    float elevation;

    PNode();
    ~PNode();
    PNode(const PNode&) = delete;

public:

    glm::mat4 model;

    uint heightmap;
    uint appearance, normal;

    PNode* child[4];
    PNode* parent;

    uint level, offset_type;
    int morton = 0;

    glm::vec2 lo, hi; // global coordinates
    glm::vec2 rlo, rhi; // relative coordinates

    bool IsSubdivided() const
    {
        return subdivided;
    }

    void SetModelMatrix(const glm::mat4& arg)
    {
        model = glm::translate(arg, this->GetShift());
        model = glm::scale(model, this->GetScale());
    }
    float Size() const
    {
        return 0.5f*(hi.x-lo.x + hi.y-lo.y);
    }
    glm::mat4 GetModelMatrix() const
    {
        return this->model;
    }
    glm::vec3 GetScale() const
    {
        return glm::vec3(hi.x-lo.x,1.0f,hi.y-lo.y);
    }
    glm::vec3 GetShift() const
    {
        return glm::vec3(lo.x,0.0f,lo.y);
    }
    glm::vec2 GetCenter() const
    {
        return 0.5f*glm::vec2(hi.x + lo.x,hi.y + lo.y);
    }
    glm::vec3 GetCenter3flat() const
    {
        return glm::vec3(0.5f*(hi.x + lo.x),0.0f,0.5f*(hi.y + lo.y));
    }
    glm::vec3 GetCenter3() const
    {
        return glm::vec3(0.5f*(hi.x + lo.x),elevation,0.5f*(hi.y + lo.y));
    }
    glm::vec3 GetRelativeShift() const
    {
        return glm::vec3(rlo.x,0.0f,rlo.y);
    }
    glm::vec3 GetRelativeCenter() const
    {
        return glm::vec3(0.5f*(rhi.x + rlo.x),0.0f,0.5f*(rhi.y + rlo.y));
    }
    void BakeHeightMap(glm::mat4 arg);
    void BakeAppearanceMap(glm::mat4 arg);
    void FixHeightmap(PNode* neighbour, int edgedir);
    void Split(glm::mat4 arg);
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
        if(subdivided)
        {
            delete child[0];
            delete child[1];
            delete child[2];
            delete child[3];
        }
        subdivided = false;
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
