#include "PNode.h"

#include <iostream>
#include <cmath>
#include <vector>
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
void PNode::Split(const glm::mat4& arg)
{
    if(!this->subdivided_)
    {
        child_[0] = new PNode;
        child_[0]->SetConnectivity<0>(this);
        child_[0]->SetModelMatrix(arg);


        child_[1] = new PNode;
        child_[1]->SetConnectivity<1>(this);
        child_[1]->SetModelMatrix(arg);


        child_[2] = new PNode;
        child_[2]->SetConnectivity<2>(this);
        child_[2]->SetModelMatrix(arg);

        child_[3] = new PNode;
        child_[3]->SetConnectivity<3>(this);
        child_[3]->SetModelMatrix(arg);

        this->subdivided_ = true;
    }
}
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
    RenderGrid();
}


//// GeoNode class implementation
Shader PGeoNode::upsampling;
Shader PGeoNode::crackfixing;
Shader PGeoNode::appearance_baking;
uint PGeoNode::noiseTex;
uint PGeoNode::elevationTex;
uint PGeoNode::materialTex;
bool PGeoNode::USE_CACHE = true;
std::vector<std::tuple<uint,uint,uint>> PGeoNode::CACHE;

PGeoNode::PGeoNode() : PNode()
{
    QueryTextureHandle();
}

PGeoNode::~PGeoNode()
{
    ReleaseTextureHandle();
}

void PGeoNode::BakeAppearanceMap(const glm::mat4& arg)
{

    //Datafield//
    //Store the volume data to polygonise
    glEnable(GL_TEXTURE_2D);

    appearance_baking.use();
    appearance_baking.setMat4("globalMatrix", arg);
    appearance_baking.setInt("level", this->level_);
    appearance_baking.setInt("hash", this->morton_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, elevationTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D_ARRAY, materialTex);

    // write to heightmap ? buggy
    glBindImageTexture(0, appearance_, 0, GL_FALSE, 0, GL_WRITE_ONLY, APPEARANCE_MAP_INTERNAL_FORMAT);
    glBindImageTexture(1, normal_, 0, GL_FALSE, 0, GL_WRITE_ONLY, APPEARANCE_MAP_INTERNAL_FORMAT);

    // Deploy kernel
    glDispatchCompute((ALBEDO_MAP_X/16)+1,(ALBEDO_MAP_Y/16)+1,1);

}
void PGeoNode::BakeHeightMap(const glm::mat4& arg)
{
    // Initialize 2d heightmap texture

    //Datafield//
    //Store the volume data to polygonise
    glEnable(GL_TEXTURE_2D);

    upsampling.use();
    upsampling.setMat4("globalMatrix", arg);
    upsampling.setInt("level", this->level_);
    upsampling.setInt("hash", this->morton_);

    // bind height map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightmap_);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, elevationTex);

    // write to heightmap ? buggy
    glBindImageTexture(0, heightmap_, 0, GL_FALSE, 0, GL_WRITE_ONLY, HEIGHT_MAP_INTERNAL_FORMAT);

    // Deploy kernel
    glDispatchCompute((HEIGHT_MAP_X/16)+1,(HEIGHT_MAP_Y/16)+1,1);

    // Write flag
    crackfixed_ = false;

}
void PGeoNode::FixHeightMap(PNode* neighbour, int edgedir)
{
    // edge direction
    // 0: left, 1: top, 2:right, 3:bottom
    glm::vec2 begin,end;
    glm::vec2 my_begin,my_end;
    glm::vec2 texrange;

    //printf(" block range (%f,%f), (%f,%f)\n",lo.x,lo.y, hi.x, hi.y);

    if(edgedir == 0)
    {
        texrange.x = (lo_.y-neighbour->lo_.y)/(neighbour->hi_.y-neighbour->lo_.y);
        texrange.y = (hi_.y-neighbour->lo_.y)/(neighbour->hi_.y-neighbour->lo_.y);
        // scale tex.y
        begin = glm::vec2(1, (lo_.y-neighbour->lo_.y)/(neighbour->hi_.y-neighbour->lo_.y));
        end   = glm::vec2(1, (hi_.y-neighbour->lo_.y)/(neighbour->hi_.y-neighbour->lo_.y));

        my_begin = glm::vec2(0,0);
        my_end   = glm::vec2(0,1);

        //printf(" trim to left neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
    }
    if(edgedir == 2)
    {
        texrange.x = (lo_.y-neighbour->lo_.y)/(neighbour->hi_.y-neighbour->lo_.y);
        texrange.y = (hi_.y-neighbour->lo_.y)/(neighbour->hi_.y-neighbour->lo_.y);
        // scale tex.y
        begin = glm::vec2(0, (lo_.y-neighbour->lo_.y)/(neighbour->hi_.y-neighbour->lo_.y));
        end   = glm::vec2(0, (hi_.y-neighbour->lo_.y)/(neighbour->hi_.y-neighbour->lo_.y));

        my_begin = glm::vec2(1,0);
        my_end   = glm::vec2(1,1);

        //printf(" trim to right neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
    }
    if(edgedir == 1)
    {
        texrange.x = (lo_.x-neighbour->lo_.x)/(neighbour->hi_.x-neighbour->lo_.x);
        texrange.y = (hi_.x-neighbour->lo_.x)/(neighbour->hi_.x-neighbour->lo_.x);
        // scale tex.x
        begin = glm::vec2((lo_.x-neighbour->lo_.x)/(neighbour->hi_.x-neighbour->lo_.x), 0);
        end   = glm::vec2((hi_.x-neighbour->lo_.x)/(neighbour->hi_.x-neighbour->lo_.x), 0);

        my_begin = glm::vec2(0,1);
        my_end   = glm::vec2(1,1);

        //printf(" trim to top neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
    }
    if(edgedir == 3)
    {
        texrange.x = (lo_.x-neighbour->lo_.x)/(neighbour->hi_.x-neighbour->lo_.x);
        texrange.y = (hi_.x-neighbour->lo_.x)/(neighbour->hi_.x-neighbour->lo_.x);
        // scale tex.x
        begin = glm::vec2((lo_.x-neighbour->lo_.x)/(neighbour->hi_.x-neighbour->lo_.x), 1);
        end   = glm::vec2((hi_.x-neighbour->lo_.x)/(neighbour->hi_.x-neighbour->lo_.x), 1);

        my_begin = glm::vec2(0,0);
        my_end   = glm::vec2(1,0);

        //printf(" trim to bottom neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
    }

    // notice: assumed square height map (hx = hy = 17)
    crackfixing.use();
    crackfixing.setVec2("mylo", my_begin);
    crackfixing.setVec2("myhi", my_end);
    crackfixing.setVec2("shlo", begin);
    crackfixing.setVec2("shhi", end);

    // bind neighbour heightmap
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ((PGeoNode*)neighbour)->heightmap_);

    // write to heightmap
    glBindImageTexture(0, heightmap_, 0, GL_FALSE, 0, GL_WRITE_ONLY, HEIGHT_MAP_INTERNAL_FORMAT);

    // Deploy kernel
    glDispatchCompute(1,1,1);

    // make sure writing to image has finished before read
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Write flag
    crackfixed_ = true;
}
void PGeoNode::Split(const glm::mat4& arg)
{
    if(!this->subdivided_)
    {
        child_[0] = new PGeoNode;
        child_[0]->SetConnectivity<0>(this);
        child_[0]->SetModelMatrix(arg);
        ((PGeoNode*)child_[0])->BakeHeightMap(arg);
        ((PGeoNode*)child_[0])->BakeAppearanceMap(arg);
        ((PGeoNode*)child_[0])->SetElevation();


        child_[1] = new PGeoNode;
        child_[1]->SetConnectivity<1>(this);
        child_[1]->SetModelMatrix(arg);
        ((PGeoNode*)child_[1])->BakeHeightMap(arg);
        ((PGeoNode*)child_[1])->BakeAppearanceMap(arg);
        ((PGeoNode*)child_[1])->SetElevation();


        child_[2] = new PGeoNode;
        child_[2]->SetConnectivity<2>(this);
        child_[2]->SetModelMatrix(arg);
        ((PGeoNode*)child_[2])->BakeHeightMap(arg);
        ((PGeoNode*)child_[2])->BakeAppearanceMap(arg);
        ((PGeoNode*)child_[2])->SetElevation();


        child_[3] = new PGeoNode;
        child_[3]->SetConnectivity<3>(this);
        child_[3]->SetModelMatrix(arg);
        ((PGeoNode*)child_[3])->BakeHeightMap(arg);
        ((PGeoNode*)child_[3])->BakeAppearanceMap(arg);
        ((PGeoNode*)child_[3])->SetElevation();


        this->subdivided_ = true;
    }
}

float PGeoNode::MinElevation() const
{
#if 1
    std::vector<float> heightData(HEIGHT_MAP_X*HEIGHT_MAP_Y);

    // read texture
    glGetTextureImage(heightmap_, 0, GL_RED, GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&heightData[0]);

    auto min = std::min_element(heightData.begin(),heightData.end());
    return *min;
#else
    return 0;
#endif
}
float PGeoNode::GetElevation(const glm::vec2& pos) const
{
    // pos must located inside this grid
    // use queryGrid() before call this function
    // Caution: getTextureImage is SUPER HEAVY
    // so use this function as few as possible
    // and avoid to call this function every frame
    glm::vec2 relPos = (pos-lo_)/(hi_-lo_);

    uint xoffset = uint(glm::clamp(relPos.x,0.0f,1.0f)*(HEIGHT_MAP_X-1));
    uint yoffset = uint(glm::clamp(relPos.y,0.0f,1.0f)*(HEIGHT_MAP_Y-1));

    float height = 0.0f;
    glGetTextureSubImage(heightmap_,
                         0,xoffset,yoffset,0,1,1,1,GL_RED,GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&height);

    return height;
}
void PGeoNode::SetElevation()
{
    elevation_ = GetElevation(GetCenter());
}

void PGeoNode::QueryTextureHandle()
{
    if(!texture_handle_allocated_)
    {
        if(PGeoNode::CACHE.empty() || !PGeoNode::USE_CACHE)
        {
            glGenTextures(1, &heightmap_);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, heightmap_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            //Generate a distance field to the center of the cube
            glTexImage2D( GL_TEXTURE_2D, 0, HEIGHT_MAP_INTERNAL_FORMAT, HEIGHT_MAP_X, HEIGHT_MAP_Y, 0, HEIGHT_MAP_FORMAT, GL_FLOAT, NULL);

            glGenTextures(1, &appearance_);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, appearance_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            //Generate a distance field to the center of the cube
            glTexImage2D( GL_TEXTURE_2D, 0, APPEARANCE_MAP_INTERNAL_FORMAT, ALBEDO_MAP_X, ALBEDO_MAP_Y, 0, GL_RGBA, GL_FLOAT, NULL);

            glGenTextures(1, &normal_);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, normal_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            //Generate a distance field to the center of the cube
            glTexImage2D( GL_TEXTURE_2D, 0, APPEARANCE_MAP_INTERNAL_FORMAT, ALBEDO_MAP_X, ALBEDO_MAP_Y, 0, GL_RGBA, GL_FLOAT, NULL);

        }
        else
        {
            heightmap_ = std::get<0>(PGeoNode::CACHE.back());
            appearance_ = std::get<1>(PGeoNode::CACHE.back());
            normal_ = std::get<2>(PGeoNode::CACHE.back());
            PGeoNode::CACHE.pop_back();
        }
    }

    texture_handle_allocated_ = true;

}
void PGeoNode::ReleaseTextureHandle()
{
    if(texture_handle_allocated_)
    {
        if(PGeoNode::CACHE.size() > MAX_CACHE_CAPACITY || !PGeoNode::USE_CACHE)
        {
            glDeleteTextures(1, &heightmap_);
            glDeleteTextures(1, &appearance_);
            glDeleteTextures(1, &normal_);
        }
        else
        {
            PGeoNode::CACHE.push_back(std::make_tuple(heightmap_,appearance_,normal_));
        }
    }

    texture_handle_allocated_ = false;
}

void PGeoNode::Init()
{
    //void CubeAssetTilesInit()
    {
        elevationTex = loadCubemapLarge(FP("../../resources/Earth/Bump/"),".png", 1);
    }

    materialTex = loadLayeredTexture("Y42lf.png",FP("../../resources/textures"), false);

    // Geo mesh, careful: need a noise texture and shader before intialized
    upsampling.reload_shader_program_from_files(FP("renderer/upsampling.glsl"));
    appearance_baking.reload_shader_program_from_files(FP("renderer/appearance.glsl"));
    crackfixing.reload_shader_program_from_files(FP("renderer/crackfixing.glsl"));
}
void PGeoNode::Finalize()
{
    //glDeleteTextures(1,&noiseTex);
    glDeleteTextures(1,&elevationTex);
    if(!CACHE.empty())
    {
        for(auto i: CACHE)
        {
            glDeleteTextures(1,&std::get<0>(i));
            glDeleteTextures(1,&std::get<1>(i));
            glDeleteTextures(1,&std::get<2>(i));
        }
    }
    CACHE.clear();
}

// renderGrid() renders a 16x16 2d grid in NDC.
// -------------------------------------------------
uint PNode::gridVAO = 0;
uint PNode::gridVBO = 0;

void PNode::RenderGrid()
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

                vertices.push_back(float8( lo.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, lo.x, lo.y)); // bottom-left
                vertices.push_back(float8( lo.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, lo.x, hi.y)); // top-left
                vertices.push_back(float8( hi.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, hi.x, hi.y)); // top-right

                vertices.push_back(float8( hi.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, hi.x, hi.y)); // top-right
                vertices.push_back(float8( hi.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, hi.x, lo.y)); // bottom-right
                vertices.push_back(float8( lo.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, lo.x, lo.y)); // bottom-left

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

#include "imgui.h"

void PGeoNode::GuiInterface()
{                       // Create a window called "Hello, world!" and append into it.

    //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("PNode::Control Panel"))
    {
        ImGui::Text("Controllable parameters for PNode class.");               // Display some text (you can use a format strings too)

        ImGui::Checkbox("Use cache", &PGeoNode::USE_CACHE);
        if(PGeoNode::USE_CACHE)
        {
            ImGui::Text("cache load %d", PGeoNode::CACHE.size());
        }
        ImGui::Text("Number of PNodes generated %d", PGeoNode::NODE_COUNT);
        ImGui::Text("Number of interface PNodes generated %d", PGeoNode::INTERFACE_NODE_COUNT);

        if (ImGui::TreeNode("Noise map"))
        {
            ImGuiIO& io = ImGui::GetIO();
            //ImGui::TextWrapped("Below we are displaying the font texture (which is the only texture we have access to in this demo). Use the 'ImTextureID' type as storage to pass pointers or identifier to your own texture data. Hover the texture for a zoomed view!");

            // Here we are grabbing the font texture because that's the only one we have access to inside the demo code.
            // Remember that ImTextureID is just storage for whatever you want it to be, it is essentially a value that will be passed to the render function inside the ImDrawCmd structure.
            // If you use one of the default imgui_impl_XXXX.cpp renderer, they all have comments at the top of their file to specify what they expect to be stored in ImTextureID.
            // (for example, the imgui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer. The imgui_impl_opengl3.cpp renderer expect a GLuint OpenGL texture identifier etc.)
            // If you decided that ImTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers to ImGui::Image(), and gather width/height through your own functions, etc.
            // Using ShowMetricsWindow() as a "debugger" to inspect the draw data that are being passed to your render will help you debug issues if you are confused about this.
            // Consider using the lower-level ImDrawList::AddImage() API, via ImGui::GetWindowDrawList()->AddImage().
            ImTextureID my_tex_id = (GLuint*)noiseTex;
            float my_tex_w = (float)256;
            float my_tex_h = (float)256;

            ImGui::Text("%.0fx%.0f", my_tex_w, my_tex_h);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::Image(my_tex_id, ImVec2(my_tex_w, my_tex_h), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                float region_sz = 32.0f;
                float region_x = io.MousePos.x - pos.x - region_sz * 0.5f; if (region_x < 0.0f) region_x = 0.0f; else if (region_x > my_tex_w - region_sz) region_x = my_tex_w - region_sz;
                float region_y = io.MousePos.y - pos.y - region_sz * 0.5f; if (region_y < 0.0f) region_y = 0.0f; else if (region_y > my_tex_h - region_sz) region_y = my_tex_h - region_sz;
                float zoom = 4.0f;
                ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
                ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
                ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
                ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
                ImGui::Image(my_tex_id, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::EndTooltip();
            }

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Elevation map"))
        {
            ImGuiIO& io = ImGui::GetIO();
            //ImGui::TextWrapped("Below we are displaying the font texture (which is the only texture we have access to in this demo). Use the 'ImTextureID' type as storage to pass pointers or identifier to your own texture data. Hover the texture for a zoomed view!");

            // Here we are grabbing the font texture because that's the only one we have access to inside the demo code.
            // Remember that ImTextureID is just storage for whatever you want it to be, it is essentially a value that will be passed to the render function inside the ImDrawCmd structure.
            // If you use one of the default imgui_impl_XXXX.cpp renderer, they all have comments at the top of their file to specify what they expect to be stored in ImTextureID.
            // (for example, the imgui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer. The imgui_impl_opengl3.cpp renderer expect a GLuint OpenGL texture identifier etc.)
            // If you decided that ImTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers to ImGui::Image(), and gather width/height through your own functions, etc.
            // Using ShowMetricsWindow() as a "debugger" to inspect the draw data that are being passed to your render will help you debug issues if you are confused about this.
            // Consider using the lower-level ImDrawList::AddImage() API, via ImGui::GetWindowDrawList()->AddImage().
            ImTextureID my_tex_id = (GLuint*)elevationTex;
            float my_tex_w = (float)256;
            float my_tex_h = (float)256;

            ImGui::Text("%.0fx%.0f", my_tex_w, my_tex_h);
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImGui::Image(my_tex_id, ImVec2(my_tex_w, my_tex_h), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                float region_sz = 32.0f;
                float region_x = io.MousePos.x - pos.x - region_sz * 0.5f; if (region_x < 0.0f) region_x = 0.0f; else if (region_x > my_tex_w - region_sz) region_x = my_tex_w - region_sz;
                float region_y = io.MousePos.y - pos.y - region_sz * 0.5f; if (region_y < 0.0f) region_y = 0.0f; else if (region_y > my_tex_h - region_sz) region_y = my_tex_h - region_sz;
                float zoom = 4.0f;
                ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
                ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
                ImVec2 uv0 = ImVec2((region_x) / my_tex_w, (region_y) / my_tex_h);
                ImVec2 uv1 = ImVec2((region_x + region_sz) / my_tex_w, (region_y + region_sz) / my_tex_h);
                ImGui::Image(my_tex_id, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 0.5f));
                ImGui::EndTooltip();
            }

            ImGui::TreePop();
        }
        ImGui::TreePop();
    }


}
