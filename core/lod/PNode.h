#ifndef PNODE_H
#define PNODE_H
#include <iostream>
#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <memory>
#include <tuple>
#include <map>
#include <list>
typedef unsigned int uint;
class Shader;

// Node class
// implements basic tree operations
class PNode
{
protected:
    static unsigned int gridVAO;
    static unsigned int gridVBO;
    static void RenderGridXY();

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
        model_ = glm::translate(arg, this->GetShiftXY());
        model_ = glm::scale(model_, this->GetScaleU());
    }
    float Size() const
    {
        return 0.5f*(hi_.x-lo_.x + hi_.y-lo_.y);
    }
    glm::mat4 GetModelMatrix() const
    {
        return this->model_;
    }
    glm::vec3 GetScaleA() const
    {
        return glm::vec3(hi_.x-lo_.x,1.0f,hi_.y-lo_.y);
    }
    glm::vec3 GetShiftXZ() const
    {
        return glm::vec3(lo_.x,0.0f,lo_.y);
    }
    glm::vec3 GetShiftXY() const
    {
        return glm::vec3(lo_.x,lo_.y,0.0f);
    }
    glm::vec3 GetScaleU() const
    {
        return glm::vec3(2.0/(1<<level_));
    }
    glm::vec2 GetCenter() const
    {
        return 0.5f*glm::vec2(hi_.x + lo_.x,hi_.y + lo_.y);
    }
    glm::vec3 GetCenter3flat() const
    {
        return glm::vec3(0.5f*(hi_.x + lo_.x),0.0f,0.5f*(hi_.y + lo_.y));
    }
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
    virtual void Split(const glm::mat4& arg) = 0;
    int Search(glm::vec2 p) const;
    PNode* Query(uint) const;

    // Generic functions
    template <typename Func>
    void Exec(Func&& func) {
        if(func(this))
            for(auto id_: child_)
                id_->Exec(func);
    }

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
    static std::vector<std::vector<PNode*>> GetLevelOrder(PNode* root);

    // static member
    static uint NODE_COUNT;
    static uint INTERFACE_NODE_COUNT;

    static constexpr int GRIDX =(17);
    static constexpr int GRIDY =(17);

};

class FieldData2D
{
public:
    const char* glsl_name_;
    GLuint glsl_entry_;

    FieldData2D(){}
    FieldData2D(const char* glsl_name, GLuint glsl_entry)
        : glsl_name_(glsl_name)
        , glsl_entry_(glsl_entry)
    {}
    ~FieldData2D(){ ReleaseBuffers(); }
    //FieldData2D(const FieldData2D&) = delete;

    uint GetReadBuffer() const { return ptr_[0]; }
    uint GetWriteBuffer() const { return ptr_[1]; }
    void SwapBuffer() { std::swap(ptr_[0],ptr_[1]); }
    void BindTexture(int i = 0);
    void BindImage(int i = 1);
    void BindDefault();
    void AllocBuffers(int w, int h, float* data = nullptr);
    void ReleaseBuffers();

protected:
    GLuint texture_type_ = GL_TEXTURE_2D;
    GLuint internal_format_ = GL_RGBA32F;
    GLuint format_ = GL_RGBA;

    GLuint ptr_[2];
};

class AMRNode : public PNode
{
protected:
    using dataStorage = std::map<std::string, FieldData2D>;
    using shaderStorage = std::map<std::string, Shader>;

protected:
    // Storage for allocated fields
    dataStorage fields_;

    // Constructor&Destructor
    AMRNode() : fields_(AMRNode::kFieldList) {}
    virtual ~AMRNode() {}
    AMRNode(const AMRNode&) = delete;

public:
    // operations
    void AssignField();
    void Streaming();
    void Collision();
    void InterpCoarseToFine();
    void InterpFineToCoarse();
    void BoundaryPatch();

    // AMR operator
    // Implement user-defined behaviour
    virtual void Split(const glm::mat4& arg) override;

    // may not necessary as we have FieldData class to manage the mem allocation
    //void QueryTextureHandle();
    //void ReleaseTextureHandle();

    // output
    void BindRenderTarget(const char*);

    // Static function
    static void Init();
    static void Finalize();
    static void GuiInterface();

    static void RegisterComputeShader(const char* name, const char* path);
    static void RegisterField(const char* glsl_name, uint glsl_entry);
    const dataStorage& GetFields() const;
    dataStorage* GetFieldPtr();
    template <typename ForwardFunc, typename BackwardFunc>
    static void MultiLevelIntegrator(
            ForwardFunc&& f,
            BackwardFunc&& g,
            const std::vector<std::vector<PNode*>>& list,
            uint level )
    {
        if(level >= list.size())
            return;

        f(list[level],level);

        MultiLevelIntegrator(f, g, list, level+1);
        MultiLevelIntegrator(f, g, list, level+1);

        g(list[level],level);
    }

    // Static member
    static constexpr int NUM_GHOST_LAYER = 2;
    static constexpr int FIELD_MAP_X =(28+2*NUM_GHOST_LAYER); // -1: node based to cell based
    static constexpr int FIELD_MAP_Y =(28+2*NUM_GHOST_LAYER);

    static constexpr auto FIELD_MAP_INTERNAL_FORMAT = GL_RGBA32F;
    static constexpr auto FIELD_MAP_FORMAT = GL_RGBA;

    // store all compute shaders needed
    static shaderStorage kShaderList;

    // store all uniform fields needed
    static dataStorage kFieldList;
};
#endif
