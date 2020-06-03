#include "grid.h"

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include "shader.h"
#include "cmake_source_dir.h"

#include <stdint.h>

#define HEIGHT_MAP_INTERNAL_FORMAT GL_RGBA32F
#define HEIGHT_MAP_FORMAT GL_RGBA

static Shader upsampling, crackfixing;
static unsigned int noiseTex, elevationTex;


uint Node::NODE_COUNT = 0;
uint Node::INTERFACE_NODE_COUNT = 0;
bool Node::USE_CACHE = true;

#define MAX_CACHE_CAPACITY (1524)
std::vector<uint> Node::CACHE;


void renderGrid();

void Node::init()
{
    //Store the volume data to polygonise
    glGenTextures(1, &noiseTex);
    glActiveTexture(GL_TEXTURE0);
    //glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, noiseTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    //Generate a distance field to the center of the cube
    std::vector<glm::vec3> dataField;
    uint res = 256;
    for(uint j=0; j<res; j++){
        for(uint i=0; i<res; i++){
            dataField.push_back( glm::vec3(rand() / double(RAND_MAX)) );
        }
    }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, res, res, 0, GL_RGB, GL_FLOAT, &dataField[0]);

    // prescribed elevation map
    glGenTextures(1, &elevationTex);
    glActiveTexture(GL_TEXTURE0);
    //glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, elevationTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    //Generate a distance field to the center of the cube
    dataField.clear();
    res = 256;
    for(uint j=0; j<res; j++){
        for(uint i=0; i<res; i++){
            //if(i > res/3)
            //    dataField.push_back( 0.5*(tanh(0.02f*(i - res/3)-1)) );
            //else
            //    dataField.push_back(0);
            glm::vec3 data( -6.0*tanh(0.005*(10*((i - res/2.0)*(i - res/2.0) + (j- res/2.0)*(j - res/2.0))/(float)res/(float)res-1.7)) );
            dataField.push_back( data );
        }
    }

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, res, res, 0, GL_RGB, GL_FLOAT, &dataField[0]);

    // Geo mesh, careful: need a noise texture and shader before intialized
    upsampling.reload_shader_program_from_files(FP("renderer/upsampling.glsl"));
    crackfixing.reload_shader_program_from_files(FP("renderer/crackfixing.glsl"));

    return;
}

void Node::finalize()
{
    glDeleteTextures(1,&noiseTex);
    glDeleteTextures(1,&elevationTex);
    if(!CACHE.empty())
        glDeleteTextures(1,&Node::CACHE[0]);
    CACHE.clear();
}

Node::Node() : lo(-1), hi(1), rlo(0), rhi(1), subdivided(false), crackfixed(false), level(0), offset_type(0),elevation(0)
{
    if(Node::CACHE.empty() || !Node::USE_CACHE)
    {
        glGenTextures(1, &heightmap);

        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, heightmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        //Generate a distance field to the center of the cube
        glTexImage2D( GL_TEXTURE_2D, 0, HEIGHT_MAP_INTERNAL_FORMAT, HEIGHT_MAP_X, HEIGHT_MAP_Y, 0, HEIGHT_MAP_FORMAT, GL_FLOAT, NULL);
    }
    else
    {
        heightmap = Node::CACHE.back();
        Node::CACHE.pop_back();
    }

    //glGenTextures(1, &appearance);
    Node::NODE_COUNT++;

}

Node::~Node()
{
    if(subdivided)
    {
        delete child[0];
        delete child[1];
        delete child[2];
        delete child[3];
    }

    if(Node::CACHE.size() > MAX_CACHE_CAPACITY || !Node::USE_CACHE)
    {
        glDeleteTextures(1, &heightmap);
    }
    else
    {
        Node::CACHE.push_back(heightmap);
    }

    //glDeleteTextures(1, &appearance);
    Node::NODE_COUNT--;
}

void Node::draw()
{
    // Render grid
    renderGrid();
}

void Node::bake_height_map(glm::mat4 arg)
{
    // Initialize 2d heightmap texture

    //Datafield//
    //Store the volume data to polygonise
    glEnable(GL_TEXTURE_2D);

    upsampling.use();
    upsampling.setVec2("lo", lo);
    upsampling.setVec2("hi", hi);

    upsampling.setMat4("globalMatrix", arg);

    // bind height map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightmap);

    // bind noisemap
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, noiseTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, elevationTex);

    // write to heightmap ? buggy
    glBindImageTexture(0, heightmap, 0, GL_FALSE, 0, GL_WRITE_ONLY, HEIGHT_MAP_INTERNAL_FORMAT);

    // Deploy kernel
    glDispatchCompute(1,1,1);

    // make sure writing to image has finished before read
    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Write flag
    crackfixed = false;

}
void Node::fix_heightmap(Node* neighbour, int edgedir)
{
    // edge direction
    // 0: left, 1: top, 2:right, 3:bottom
    glm::vec2 begin,end;
    glm::vec2 my_begin,my_end;
    glm::vec2 texrange;

    //printf(" block range (%f,%f), (%f,%f)\n",lo.x,lo.y, hi.x, hi.y);

    if(edgedir == 0)
    {
        texrange.x = (lo.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y);
        texrange.y = (hi.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y);
        // scale tex.y
        begin = glm::vec2(1, (lo.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y));
        end   = glm::vec2(1, (hi.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y));

        my_begin = glm::vec2(0,0);
        my_end   = glm::vec2(0,1);

        //printf(" trim to left neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
    }
    if(edgedir == 2)
    {
        texrange.x = (lo.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y);
        texrange.y = (hi.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y);
        // scale tex.y
        begin = glm::vec2(0, (lo.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y));
        end   = glm::vec2(0, (hi.y-neighbour->lo.y)/(neighbour->hi.y-neighbour->lo.y));

        my_begin = glm::vec2(1,0);
        my_end   = glm::vec2(1,1);

        //printf(" trim to right neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
    }
    if(edgedir == 1)
    {
        texrange.x = (lo.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x);
        texrange.y = (hi.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x);
        // scale tex.x
        begin = glm::vec2((lo.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x), 0);
        end   = glm::vec2((hi.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x), 0);

        my_begin = glm::vec2(0,1);
        my_end   = glm::vec2(1,1);

        //printf(" trim to top neighbour, shared tex range(%f->%f)\n",texrange.x,texrange.y);
    }
    if(edgedir == 3)
    {
        texrange.x = (lo.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x);
        texrange.y = (hi.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x);
        // scale tex.x
        begin = glm::vec2((lo.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x), 1);
        end   = glm::vec2((hi.x-neighbour->lo.x)/(neighbour->hi.x-neighbour->lo.x), 1);

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
    glBindTexture(GL_TEXTURE_2D, neighbour->heightmap);

    // write to heightmap
    glBindImageTexture(0, heightmap, 0, GL_FALSE, 0, GL_WRITE_ONLY, HEIGHT_MAP_INTERNAL_FORMAT);

    // Deploy kernel
    glDispatchCompute(1,1,1);

    // make sure writing to image has finished before read
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Write flag
    crackfixed = true;
}
void Node::split(glm::mat4 arg = glm::mat4(1))
{
    if(!this->subdivided)
    {
        child[0] = new Node;
        child[0]->setconnectivity<0>(this);
        child[0]->set_model_matrix(arg);
        child[0]->bake_height_map(arg);
        child[0]->set_elevation();


        child[1] = new Node;
        child[1]->setconnectivity<1>(this);
        child[1]->set_model_matrix(arg);
        child[1]->bake_height_map(arg);
        child[1]->set_elevation();


        child[2] = new Node;
        child[2]->setconnectivity<2>(this);
        child[2]->set_model_matrix(arg);
        child[2]->bake_height_map(arg);
        child[2]->set_elevation();


        child[3] = new Node;
        child[3]->setconnectivity<3>(this);
        child[3]->set_model_matrix(arg);
        child[3]->bake_height_map(arg);
        child[3]->set_elevation();


        this->subdivided = true;
    }
}
int Node::search(glm::vec2 p) const
{
    glm::vec2 center = get_center();

    // not in box
    if(p.x < lo.x || p.y < lo.y || p.x > hi.x || p.y > hi.y) { return -1; }

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
            return 3; // bottom-right
        else
            return 2; //top-right
    }

}
float Node::min_elevation() const
{
    std::vector<float> heightData(HEIGHT_MAP_X*HEIGHT_MAP_Y);

    // read texture
    glGetTextureImage(heightmap, 0, GL_RED, GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&heightData[0]);

    auto min = std::min_element(heightData.begin(),heightData.end());
    return *min;
}
float Node::get_elevation(const glm::vec2& pos) const
{
    // pos must located inside this grid
    // use queryGrid() before call this function
    // Caution: getTextureImage is SUPER HEAVY
    // so use this function as few as possible
    // and avoid to call this function every frame
    glm::vec2 relPos = (pos-lo)/(hi-lo);

    uint xoffset = uint(glm::clamp(relPos.x,0.0f,1.0f)*(HEIGHT_MAP_X-1));
    uint yoffset = uint(glm::clamp(relPos.y,0.0f,1.0f)*(HEIGHT_MAP_Y-1));

    float height = 0.0f;
    glGetTextureSubImage(heightmap,
        0,xoffset,yoffset,0,1,1,1,GL_RED,GL_FLOAT,HEIGHT_MAP_X*HEIGHT_MAP_Y*sizeof(float),&height);

    return height;
}
void Node::set_elevation()
{
    elevation = get_elevation(get_center());
}

template<uint TYPE>
void Node::setconnectivity(Node* leaf)
{
    static_assert (TYPE < 4, "Only 4 child index" );
    offset_type = TYPE;
    glm::vec2 center = glm::vec2(0.5f)*(leaf->lo + leaf->hi);
    if(TYPE == 0) // bottom-left
    {
        lo = leaf->lo;
        hi = center;
    }else
    if(TYPE == 1) // top-left
    {
        lo = glm::vec2(leaf->lo.x, center.y);
        hi = glm::vec2(center.x, leaf->hi.y);
    }else
    if(TYPE == 2) // top-right
    {
        lo = center;
        hi = leaf->hi;
    }else
    if(TYPE == 3) // bottom-right
    {
        lo = glm::vec2(center.x, leaf->lo.y);
        hi = glm::vec2(leaf->hi.x, center.y);
    }

    parent = leaf;

    level = parent->level+1;

    rlo = (lo - parent->lo)/(parent->hi - parent->lo);
    rhi = (hi - parent->lo)/(parent->hi - parent->lo);
}

// renderGrid() renders a 16x16 2d grid in NDC.
// -------------------------------------------------
static unsigned int gridVAO = 0;
static unsigned int gridVBO = 0;

void renderGrid()
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
                glm::vec2 tlo((i + 0.5)/float(HEIGHT_MAP_X), (j + 0.5)/float(HEIGHT_MAP_Y));
                glm::vec2 thi((i + 1.5)/float(HEIGHT_MAP_X), (j + 1.5)/float(HEIGHT_MAP_Y));

                vertices.push_back(float8( lo.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, tlo.x, tlo.y)); // bottom-left
                vertices.push_back(float8( lo.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, tlo.x, thi.y)); // top-left
                vertices.push_back(float8( hi.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, thi.x, thi.y)); // top-right

                vertices.push_back(float8( hi.x, 0.0f,  hi.y,  0.0f, -1.0f,  0.0f, thi.x, thi.y)); // top-right
                vertices.push_back(float8( hi.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, thi.x, tlo.y)); // bottom-right
                vertices.push_back(float8( lo.x, 0.0f,  lo.y,  0.0f, -1.0f,  0.0f, tlo.x, tlo.y)); // bottom-left

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

void Node::gui_interface()
{                       // Create a window called "Hello, world!" and append into it.

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Node::Control Panel"))
    {
        ImGui::Text("Controllable parameters for Node class.");               // Display some text (you can use a format strings too)

        ImGui::Checkbox("Use cache", &Node::USE_CACHE);
        if(Node::USE_CACHE)
        {
            ImGui::Text("cache load %d", Node::CACHE.size());
        }
        ImGui::Text("Number of nodes generated %d", Node::NODE_COUNT);
        ImGui::Text("Number of interface nodes generated %d", Node::INTERFACE_NODE_COUNT);

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
