#include "OGeomesh.h"
#include <glad/glad.h>
#include "UCartesianMath.h"


glm::vec3 OGeomesh::ConvertToDeformed(const glm::vec3& v) const
{
    return glm::vec3(global_model_matrix_*glm::vec4(v,1.0));
}

glm::vec3 OGeomesh::ConvertToNormal(const glm::vec3& v) const
{
    return glm::vec3(glm::inverse(global_model_matrix_)*glm::vec4(v,1.0));
}

glm::vec3 OGeomesh::ConvertToUV(const glm::vec3& v) const
{
    auto nv = glm::normalize(v); // normalize to R=1
    float d;
    auto orig = ConvertToDeformed(glm::vec3(0));
    auto dir = glm::normalize(ConvertToDeformed(glm::vec3(0,1,0)));
    bool intersected = glm::intersectRayPlane	(	glm::vec3(0),
                                                    -nv,
                                                    orig,
                                                    dir,
                                                    d);

    if(!intersected)
        return glm::vec3(9999);

    nv = ConvertToNormal(nv*fabsf(d)); // project to plane then transform to +y plane
    nv.y = glm::length(v) - 1.0f;

    return nv;
}

bool OGeomesh::IsGroundReference(const glm::vec3& pos) const
{
    auto tpos = ConvertToUV(pos);
    if(abs(tpos.x)<= 1 && abs(tpos.z)<= 1)
        return true;
    return false;
}

float OGeomesh::QueryElevation(const glm::vec3& pos) const
{
    auto tpos = ConvertToUV(pos);
    auto node = (PGeoNode*)QueryNode(glm::vec2(tpos.x, tpos.z));
    if(node)
        return node->GetElevation(glm::vec2(tpos.x, tpos.z));
    return this->GetElevation(glm::vec2(tpos.x, tpos.z));
}

void OGeomesh::ReleaseAllTextureHandles()
{
    ReleaseAllTextureHandles(this);
}

void OGeomesh::ReleaseAllTextureHandles( PGeoNode* node )
{
    if(node->IsSubdivided())
    {
        ReleaseAllTextureHandles((PGeoNode*)node->child_[0]);
        ReleaseAllTextureHandles((PGeoNode*)node->child_[1]);
        ReleaseAllTextureHandles((PGeoNode*)node->child_[2]);
        ReleaseAllTextureHandles((PGeoNode*)node->child_[3]);
    }

    node->ReleaseTextureHandle();
}

// Caution: only return subdivided grids.
// write additional condition if you need root
PNode* OGeomesh::QueryNode( const glm::vec2& pos) const
{
    //bool isTraversible = PNode->subdivided;
        // find the PNode
        auto sh_PNode = GetHandle();
        int result = -1;
        while(true)
        {
            // start from root
            result = sh_PNode->Search(pos);
            if(result != -1 && sh_PNode->IsSubdivided())
            {
                sh_PNode = sh_PNode->child_[result];
                continue;
            }
            break;
        }

        if(result == -1) { return NULL; }
        return sh_PNode;
}

void OGeomesh::RefreshHeightmap(PGeoNode* PNode)
{
    //bool isTraversible = PNode->subdivided;
    if(PNode->IsSubdivided())
    {
        RefreshHeightmap((PGeoNode*)PNode->child_[0]);
        RefreshHeightmap((PGeoNode*)PNode->child_[1]);
        RefreshHeightmap((PGeoNode*)PNode->child_[2]);
        RefreshHeightmap((PGeoNode*)PNode->child_[3]);
    }
    else
    {
        // Refresh old height map for interface cells
        //if(PNode->crackfixed)
        //{
        //    //PNode->bake_height_map();
        //}
    }
}

void OGeomesh::Fixcrack(PGeoNode* leaf)
{
    //bool isTraversible = PNode->subdivided;
    if(leaf->IsSubdivided())
    {
        Fixcrack((PGeoNode*)leaf->child_[0]);
        Fixcrack((PGeoNode*)leaf->child_[1]);
        Fixcrack((PGeoNode*)leaf->child_[2]);
        Fixcrack((PGeoNode*)leaf->child_[3]);
    }
    else
    {
        // fix crack
        // for each block, we only need to check 2 directions
        // because the other 2 directions are filled by same or higher level_ lod
        // once found a match, the height map of current block will be modified
        // only perform this to current draw level_

        //if(PNode -> crackfixed == true) { return; }

        // marks: 0: left, 1: top, 2:right, 3:bottom

        glm::vec2 f1, f2;
        int e1, e2;
        if(leaf->GetOffsetType() == 0) // going to search 2 faces
        {
            f1 = leaf->GetCenter() - glm::vec2(0.0,(leaf->hi_.y-leaf->lo_.y)); // bottom
            f2 = leaf->GetCenter() - glm::vec2((leaf->hi_.x-leaf->lo_.x), 0.0); // left
            e1 = 3;e2 = 0;
        }else
        if(leaf->GetOffsetType() == 1) // going to search 2 faces
        {
            f1 = leaf->GetCenter() + glm::vec2(0.0,(leaf->hi_.y-leaf->lo_.y)); // top
            f2 = leaf->GetCenter() - glm::vec2((leaf->hi_.x-leaf->lo_.x), 0.0); // left
            e1 = 1;e2 = 0;
        }else
        if(leaf->GetOffsetType() == 2) // going to search 2 faces
        {
            f1 = leaf->GetCenter() + glm::vec2(0.0,(leaf->hi_.y-leaf->lo_.y)); // top
            f2 = leaf->GetCenter() + glm::vec2((leaf->hi_.x-leaf->lo_.x), 0.0); // right
            e1 = 1;e2 = 2;
        }
        else
        if(leaf->GetOffsetType() == 3) // going to search 2 faces
        {
            f1 = leaf->GetCenter() - glm::vec2(0.0,(leaf->hi_.y-leaf->lo_.y)); // bottom
            f2 = leaf->GetCenter() + glm::vec2((leaf->hi_.x-leaf->lo_.x), 0.0); // right
            e1 = 3;e2 = 2;
        }
        else {return;} // may located at other blocks or out of the bound

        // find the PNode
        PNode* sh_PNode = QueryNode(f1);

        // Compare with neighbour, if my_level_ > neighbour_level_
        // a height map sync will be called here
        if(sh_PNode && leaf->level_ == (sh_PNode->level_ + 1))
        {
            leaf-> FixHeightMap(sh_PNode,e1);
        }

        // here is the second face
        sh_PNode = QueryNode(f2);

        if(sh_PNode && leaf->level_ == (sh_PNode->level_ + 1))
        {
            leaf-> FixHeightMap(sh_PNode,e2);
        }
    }
}

void OGeomesh::Subdivision(const glm::vec3& viewPos)
{
    Subdivision(ConvertToUV(viewPos), QueryElevation(viewPos), this);
}

void OGeomesh::Subdivision(uint code, uint lod)
{
    auto uv = 2.0f*Umath::DecodeMorton2(code)-1.0f;
    Subdivision(glm::vec3(uv.x, 4.0/(1<<lod), uv.y), 0, this);
}

void OGeomesh::Subdivision(uint level_)
{
    Subdivision( level_, this );
}

void OGeomesh::Subdivision(const glm::vec3& viewPos, const float& viewY, PGeoNode* leaf)
{

    // distance between PNodepos and viewpos
    //auto d = PNode->get_center3() - viewPos;
    float dx = fminf(fabsf(viewPos.x - leaf->lo_.x),fabsf(viewPos.x - leaf->hi_.x));
    float dy = fminf(fabsf(viewPos.z - leaf->lo_.y),fabsf(viewPos.z - leaf->hi_.y));
    //float dz = abs(viewPos.z - viewZ);
    float d = fmaxf(fmaxf(dx, dy), fabsf(viewPos.y - viewY));

    // frustrum culling
    float K = CUTIN_FACTOR*leaf->Size();
    //if(FRUSTRUM_CULLING)
    //{
    //    K *= glm::dot(viewFront, d) > 0.0f ? 1.0f:0.5f;
    //}

    // Subdivision
    if( leaf->level_ < MIN_DEPTH || (leaf->level_ < MAX_DEPTH && d < K) )
    {
        // split and bake heightmap
        leaf->Split(global_model_matrix_);

        Subdivision(viewPos, viewY, (PGeoNode*)leaf->child_[0]);
        Subdivision(viewPos, viewY, (PGeoNode*)leaf->child_[1]);
        Subdivision(viewPos, viewY, (PGeoNode*)leaf->child_[2]);
        Subdivision(viewPos, viewY, (PGeoNode*)leaf->child_[3]);

    }
    else
    {
        if( leaf->IsSubdivided() && d >= CUTOUT_FACTOR * K )
        {
            leaf->ReleaseConnectedNodes();
        }
    }
}

void OGeomesh::Subdivision(uint level_, PGeoNode* leaf)
{
    // Subdivision
    if( leaf->level_ < level_ && leaf->level_ < MAX_DEPTH )
    {
        // split and bake heightmap
        leaf->Split(global_model_matrix_);

        Subdivision(level_, (PGeoNode*)leaf->child_[0]);
        Subdivision(level_, (PGeoNode*)leaf->child_[1]);
        Subdivision(level_, (PGeoNode*)leaf->child_[2]);
        Subdivision(level_, (PGeoNode*)leaf->child_[3]);

    }
    else
    {
        if( leaf->IsSubdivided() )
        {
            leaf->ReleaseConnectedNodes();
        }
    }
}

glm::vec2 computeShlo(int level_, int code)
{

    code >>= (2*(level_-1));
    return 0.5f*glm::vec2((code>>1)&1, (code)&1);
}

glm::vec2 computeExactTexPos(int code)
{
    glm::vec2 o = glm::vec2(-1);
    for(int i = 0; i < 15; i++)
    {
        o += glm::vec2((code>>1)&1, (code)&1)/float(1<<i);
        code >>= 2;
    }
    return o;
}

void OGeomesh::Draw(Shader& shader, const glm::vec3& viewPos) const
{
    shader.setVec3("v3CameraProjectedPos",ConvertToUV(viewPos));
    shader.setInt("renderType", OGeomesh::RENDER_MODE);
    Draw(this, shader);
}

void OGeomesh::Draw(const PGeoNode* leaf, Shader& shader) const
{
    if(leaf->IsSubdivided())
    {
        Draw((PGeoNode*)leaf->child_[0], shader);
        Draw((PGeoNode*)leaf->child_[1], shader);
        Draw((PGeoNode*)leaf->child_[2], shader);
        Draw((PGeoNode*)leaf->child_[3], shader);
    }
    else
    {
        // Transfer local grid model
        shader.setMat4("m4CubeProjMatrix", leaf->model_);

        // Transfer lo and hi
        shader.setInt("level",leaf->level_);
        shader.setInt("hash",leaf->morton_);


        //std::cout << "current drawing PNode" << PNode->level_ << " - " << PNode->parent->level_ << std::endl;

        // Active textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, leaf->heightmap_);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ((PGeoNode*)leaf->parent_)->heightmap_);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, leaf->appearance_);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, ((PGeoNode*)leaf->parent_)->appearance_);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, leaf->normal_);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, ((PGeoNode*)leaf->parent_)->normal_);

        // Render grid (inline function call renderGrid())
        this->PNode::Draw();

        //std::cout << "ok" << std::endl;
    }
    return;
}

// static variables
uint OGeomesh::MIN_DEPTH = 0;
uint OGeomesh::MAX_DEPTH = 15;
float OGeomesh::CUTIN_FACTOR = 2.0f; // 2.8 -> see function definition
float OGeomesh::CUTOUT_FACTOR = 1.0f; // >= 1
bool OGeomesh::FRUSTRUM_CULLING = false;
bool OGeomesh::CRACK_FILLING = false;
RenderMode OGeomesh::RENDER_MODE = REAL;

#include "imgui.h"

void OGeomesh::GuiInterface()
{
    static int counter = 0;

    //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("OGeomesh::Control Panel"))
    {
        ImGui::Text("Controllable parameters for OGeomesh class.");
        ImGui::SliderInt("min depth", (int*)&MIN_DEPTH, 0, 5);
        ImGui::SliderInt("max depth", (int*)&MAX_DEPTH, 6, 15);
        ImGui::SliderFloat("cutout factor", &CUTOUT_FACTOR, 1.0f, 3.0f);

        // render mode
        const char* RENDER_TYPE_NAMES[ELEMENT_COUNT] = { "REAL", "CONTOUR", "NORMAL", "PCOLOR" };

        const char* current_element_name = (RENDER_MODE >= 0 && RENDER_MODE < ELEMENT_COUNT) ? RENDER_TYPE_NAMES[RENDER_MODE] : "Unknown";
        ImGui::SliderInt("render type", (int*)&RENDER_MODE, 0, ELEMENT_COUNT - 1, current_element_name);

        ImGui::Checkbox("frustrum culling", &FRUSTRUM_CULLING);
        ImGui::Checkbox("crack filling", &CRACK_FILLING);
        ImGui::TreePop();
    }

    //ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

    if (ImGui::TreeNode("Basic trees"))
    {
        for (int i = 0; i < 5; i++)
        {
            // Use SetNextItemOpen() so set the default state of a PNode to be open.
            // We could also use TreePNodeEx() with the ImGuiTreePNodeFlags_DefaultOpen flag to achieve the same thing!
            if (i == 0)
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);

            if (ImGui::TreeNode((void*)(intptr_t)i, "child_ %d", i))
            {
                ImGui::Text("blah blah");
                ImGui::SameLine();
                if (ImGui::SmallButton("button")) {};
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Images"))
    {
        ImGuiIO& io = ImGui::GetIO();
        //ImGui::TextWrapped("Below we are displaying the font texture (which is the only texture we have access to in this demo). Use the 'ImTextureID' type as storage to pass pointers or identifier to your own texture data. Hover the texture for a zoomed view!");

        // Here we are grabbing the font texture because that's the only one we have access to inside the demo code.
        // Remember that ImTextureID is just storage for whatever you want it to be, it is essentially a value that will be passed to the render function inside the ImDrawCmd structure.
        // If you use one of the default imgui_impl_XXXX.cpp renderer, they all have comments at the top of their file to specify what they expect to be stored in ImTextureID.
        // (for example, the imgui_impl_dx11.cpp renderer expect a 'ID3D11ShaderResourceView*' pointer. The imgui_impl_opengl3.cpp renderer expect a GLuint OpenGL texture identifier etc.)
        // If you decided that ImTextureID = MyEngineTexture*, then you can pass your MyEngineTexture* pointers to ImGui::Image(), and gather width/height through your own functions, etc.
        // Using ShowMetricsWindow() as a "debugger" to inspect the draw data that are being passed to your render will help you debug issues if you are confused about this.
        // Consider using the lower-level_ ImDrawList::AddImage() API, via ImGui::GetWindowDrawList()->AddImage().
        ImTextureID my_tex_id = io.Fonts->TexID;
        float my_tex_w = (float)io.Fonts->TexWidth;
        float my_tex_h = (float)io.Fonts->TexHeight;

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
        ImGui::TextWrapped("And now some textured buttons..");
        static int pressed_count = 0;
        for (int i = 0; i < 8; i++)
        {
            ImGui::PushID(i);
            int frame_padding = -1 + i;     // -1 = uses default padding
            if (ImGui::ImageButton(my_tex_id, ImVec2(32,32), ImVec2(0,0), ImVec2(32.0f/my_tex_w,32/my_tex_h), frame_padding, ImVec4(0.0f,0.0f,0.0f,1.0f)))
                pressed_count += 1;
            ImGui::PopID();
            ImGui::SameLine();
        }
        ImGui::NewLine();
        ImGui::Text("Pressed %d times.", pressed_count);
        ImGui::TreePop();
    }

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
}
