#include "PNode.h"

#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include "shader.h"
#include "cmake_source_dir.h"
#include "texture_utility.h"
#include "UCartesianMath.h"
#include <stdint.h>

uint PNode::NODE_COUNT = 0;
uint PNode::INTERFACE_NODE_COUNT = 0;

PNode::PNode() : subdivided_(false), lo_(-1), hi_(1), rlo_(0), rhi_(1)
{
    //QueryTextureHandle();
    PNode::NODE_COUNT++;
}
PNode::~PNode()
{
    if(subdivided_)
    {
        delete child_[0];
        delete child_[1];
        delete child_[2];
        delete child_[3];
    }

    //ReleaseTextureHandle();
    PNode::NODE_COUNT--;
}
//void PNode::Split(const glm::mat4& arg)
//{
//    if(!this->subdivided_)
//    {
//        child_[0] = new PNode;
//        child_[0]->SetConnectivity<0>(this);
//        child_[0]->SetModelMatrix(arg);
//
//
//        child_[1] = new PNode;
//        child_[1]->SetConnectivity<1>(this);
//        child_[1]->SetModelMatrix(arg);
//
//
//        child_[2] = new PNode;
//        child_[2]->SetConnectivity<2>(this);
//        child_[2]->SetModelMatrix(arg);
//
//        child_[3] = new PNode;
//        child_[3]->SetConnectivity<3>(this);
//        child_[3]->SetModelMatrix(arg);
//
//        this->subdivided_ = true;
//    }
//}
int PNode::Search(glm::vec2 p) const
{
    glm::vec2 center = GetCenter();

    // not in box
    if(p.x < lo_.x || p.y < lo_.y || p.x > hi_.x || p.y > hi_.y) { return -1; }

    // find in which subregion
    if(p.x < center.x)
    {
        if(p.y < center.y)
            return 0; // bottom-left
        else
            return 1; //top-left
    }else
    {
        if(p.y < center.y)
            return 2; // bottom-right
        else
            return 3; //top-right
    }

}
template<uint TYPE>
void PNode::SetConnectivity(PNode* leaf)
{
    static_assert (TYPE < 4, "Only have 4 child." );

    this->parent_ = leaf;
    this->level_ = parent_->level_+1;
    this->morton_ = (parent_->morton_ << 2) | TYPE;

    glm::vec2 center = glm::vec2(0.5f)*(parent_->lo_ + parent_->hi_);

    // may need for rounding error free lo and hi
    // todo: need further observation...
    switch (TYPE)
    {
    case 0x0:
        lo_ = parent_->lo_;
        hi_ = center;
        break;
    case 0x1:
        lo_ = glm::vec2(center.x, parent_->lo_.y);
        hi_ = glm::vec2(parent_->hi_.x, center.y);
        break;
    case 0x2:
        lo_ = glm::vec2(parent_->lo_.x, center.y);
        hi_ = glm::vec2(center.x, parent_->hi_.y);
        break;
    case 0x3:
        lo_ = center;
        hi_ = parent_->hi_;
        break;
    }

    rlo_ = (lo_ - parent_->lo_)/(parent_->hi_ - parent_->lo_);
    rhi_ = (hi_ - parent_->lo_)/(parent_->hi_ - parent_->lo_);
}
void PNode::Draw()
{
    // Render grid
    RenderGridXY();
}
std::vector<std::vector<PNode*>> PNode::GetLevelOrder(PNode* root)
{
    std::vector< std::vector<PNode*> > ans;
    if (root == NULL)
        return std::vector<std::vector<PNode*>>();

    std::list<PNode*> queue;
    queue.push_back(root);

    // level-order traversal
    while(!queue.empty())
    {
        std::vector<PNode*> list;
        int size = queue.size();
        for(int i = 0; i < size; i++)
        {
            PNode* node = queue.front();
            queue.pop_front();
            list.push_back(node);

            if(node->IsSubdivided())
                for(auto id_: node->child_){
                    queue.push_back(id_);
                }
        }
        ans.push_back(list);
    }

    return ans;
}


// renderGrid() renders a 16x16 2d grid in NDC.
// -------------------------------------------------
uint PNode::gridVAO = 0;
uint PNode::gridVBO = 0;

void PNode::RenderGridXY()
{

    // initialize (if necessary)
    if (gridVAO == 0)
    {
        struct float8
        {
            float val[8];
            float8(float  v1, float v2, float v3, float v4,float  v5, float v6, float v7, float v8)
            {val[0] = v1;val[1] = v2;val[2] = v3;val[3] = v4;val[4] = v5;val[5] = v6;val[6] = v7;val[7] = v8;}
        };
        std::vector<float8> vertices;

        for(int j = 0; j < GRIDY; j++)
            for(int i = 0; i < GRIDX; i++)
            {
                glm::vec2 lo((i)/float(GRIDX), (j)/float(GRIDY));
                glm::vec2 hi((i+1)/float(GRIDX), (j+1)/float(GRIDY));

                vertices.push_back(float8( lo.x, lo.y, 0.0f,  0.0f, 0.0f,  1.0f, lo.x, lo.y)); // bottom-left
                vertices.push_back(float8( hi.x, hi.y, 0.0f,  0.0f, 0.0f,  1.0f, hi.x, hi.y)); // top-right
                vertices.push_back(float8( lo.x, hi.y, 0.0f,  0.0f, 0.0f,  1.0f, lo.x, hi.y)); // top-left

                vertices.push_back(float8( hi.x, hi.y, 0.0f,  0.0f, 0.0f,  1.0f, hi.x, hi.y)); // top-right
                vertices.push_back(float8( lo.x, lo.y, 0.0f,  0.0f, 0.0f,  1.0f, lo.x, lo.y)); // bottom-left
                vertices.push_back(float8( hi.x, lo.y, 0.0f,  0.0f, 0.0f,  1.0f, hi.x, lo.y)); // bottom-right

            }

        std::cout <<  std::endl;

        glGenVertexArrays(1, &gridVAO);
        glGenBuffers(1, &gridVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferData(GL_ARRAY_BUFFER, 8*sizeof(float)*vertices.size(), &vertices[0], GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(gridVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Grid
    glBindVertexArray(gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6*GRIDX*GRIDY);
    glBindVertexArray(0);
}

void FieldData2D::BindTexture(int i)
{
    //shader->setInt(glsl_name_, glsl_entry_);
    glActiveTexture(GL_TEXTURE0+glsl_entry_);
    glBindTexture(texture_type_, ptr_[i]);
    //std::cout << "Bind Texture " << glsl_name_ << " to " << glsl_entry_ << std::endl;
}
void FieldData2D::BindImage(int i)
{
    glBindImageTexture(glsl_entry_, ptr_[i], 0, GL_FALSE, 0, GL_WRITE_ONLY, internal_format_);
}
void FieldData2D::BindDefault()
{
    //shader->setInt(glsl_name_, glsl_entry_);
    glActiveTexture(GL_TEXTURE0+glsl_entry_);
    glBindTexture(texture_type_, ptr_[0]);
    glBindImageTexture(glsl_entry_, ptr_[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, internal_format_);
    //std::cout << "Bind Texture " << glsl_name_ << " to " << glsl_entry_ << std::endl;
}
void FieldData2D::AllocBuffers(int w, int h, float* data)
{
    //if(texture_type_ != GL_TEXTURE_2D) return;

    if(ptr_[0])
    {
        glGenTextures(1, &ptr_[0]);

        glActiveTexture(GL_TEXTURE10);
        glEnable(texture_type_);
        glBindTexture(texture_type_, ptr_[0]);
        glTexParameteri(texture_type_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(texture_type_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(texture_type_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(texture_type_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //Generate a distance field to the center of the cube
        glTexImage2D( texture_type_, 0, internal_format_, w, h, 0, format_, GL_FLOAT, data);
    }

    if(ptr_[1])
    {
        glGenTextures(1, &ptr_[1]);

        glActiveTexture(GL_TEXTURE11);
        glEnable(texture_type_);
        glBindTexture(texture_type_, ptr_[1]);
        glTexParameteri(texture_type_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(texture_type_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(texture_type_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(texture_type_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //Generate a distance field to the center of the cube
        glTexImage2D( texture_type_, 0, internal_format_, w, h, 0, format_, GL_FLOAT, data);
    }

}
void FieldData2D::ReleaseBuffers()
{
    // Always allocate & deallocate as a pair
    if(ptr_[0])
        glDeleteTextures(1, &ptr_[0]);
    if(ptr_[1])
        glDeleteTextures(1, &ptr_[1]);
}


// store all compute shaders needed
AMRNode::shaderStorage AMRNode::kShaderList;
AMRNode::dataStorage AMRNode::kFieldList;

void AMRNode::RegisterField(const char* glsl_name, uint glsl_entry)
{
    FieldData2D field(glsl_name, glsl_entry);
    kFieldList.insert(std::pair<const char*, FieldData2D>(glsl_name, field));
}

void AMRNode::RegisterComputeShader(const char* name, const char* path)
{
    kShaderList.insert(std::pair<const char*, Shader>(name, Shader(path)));
}

const AMRNode::dataStorage& AMRNode::GetFields() const
{
    return fields_;
}

AMRNode::dataStorage* AMRNode::GetFieldPtr()
{
    return &fields_;
}

void AMRNode::Init()
{
    // read shaders
    RegisterComputeShader("initializer",FP("compute/draw_a_sphere.glsl"));
    RegisterComputeShader("advector",FP("compute/advection.glsl"));
    RegisterComputeShader("communicator",FP("compute/patch_comm.glsl"));
    //RegisterComputeShader("streaming",FP("renderer/streaming.glsl"));
    //RegisterComputeShader("collision",FP("renderer/collision.glsl"));


    // set constants


    // register fields
    RegisterField("f0",0);
    RegisterField("f1",1);
    RegisterField("f2",2);
    RegisterField("rho",3);
    RegisterField("vel",4);
    RegisterField("patch",5);

}

void AMRNode::Split(const glm::mat4& arg)
{
    if(!this->subdivided_)
    {
        child_[0] = new AMRNode;
        child_[0]->SetConnectivity<0>(this);
        child_[0]->SetModelMatrix(arg);

        child_[1] = new AMRNode;
        child_[1]->SetConnectivity<1>(this);
        child_[1]->SetModelMatrix(arg);


        child_[2] = new AMRNode;
        child_[2]->SetConnectivity<2>(this);
        child_[2]->SetModelMatrix(arg);

        child_[3] = new AMRNode;
        child_[3]->SetConnectivity<3>(this);
        child_[3]->SetModelMatrix(arg);

        ((AMRNode*)child_[0])->AssignField();
        ((AMRNode*)child_[1])->AssignField();
        ((AMRNode*)child_[2])->AssignField();
        ((AMRNode*)child_[3])->AssignField();

        this->subdivided_ = true;
    }
}

void AMRNode::AssignField()
{
    // Initialize field ( call in instance )
    Shader* initializer = &(kShaderList.at("initializer"));

    // Debug purpose
    //static std::vector<glm::vec4> data(FIELD_MAP_X*FIELD_MAP_Y);
    //std::generate(data.begin(), data.end(), [](){ return glm::vec4(rand()/(double)RAND_MAX); });

    // bind buffers
    initializer->use();

    for(auto& id_: fields_)
    {
        id_.second.AllocBuffers(AMRNode::FIELD_MAP_X, AMRNode::FIELD_MAP_Y);
        id_.second.BindDefault();

        //std::cout << "Binding field buffer " << id_.second.glsl_name_ << std::endl;
    }


    // set constants ...
    initializer->setInt("level", this->level_);
    initializer->setInt("hash", this->morton_);


    // Dispatch kernel
    glDispatchCompute((AMRNode::FIELD_MAP_X/16)+1,(AMRNode::FIELD_MAP_Y/16)+1,1);
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    // swap buffer when finished
    for(auto& id_: fields_)
    {
        id_.second.SwapBuffer();
    }
}

void AMRNode::BindRenderTarget(const char* fieldname)
{
    fields_.at(fieldname).BindTexture();
}
