#include "AMRMesh.h"
#include <glad/glad.h>
#include "UCartesianMath.h"


// Caution: only return subdivided grids.
// write additional condition if you need root
AMRNode* AMRMesh::QueryNode( const glm::vec2& pos) const
{
    //bool isTraversible = AMRNode->subdivided;
        // find the AMRNode
        auto sh_AMRNode = GetHandle();
        int result = -1;
        while(true)
        {
            // start from root
            result = sh_AMRNode->Search(pos);
            if(result != -1 && sh_AMRNode->IsSubdivided())
            {
                sh_AMRNode = (AMRNode*)sh_AMRNode->child_[result];
                continue;
            }
            break;
        }

        if(result == -1) { return NULL; }
        return sh_AMRNode;
}


void AMRMesh::Subdivision(uint code, uint lod)
{
    auto uv = 2.0f*Umath::DecodeMorton2(code)-1.0f;
    Subdivision(glm::vec3(uv.x, 4.0/(1<<lod), uv.y), 0, this);
}


void AMRMesh::Subdivision(const glm::vec3& viewPos, const float& viewY, AMRNode* leaf)
{

    // distance between AMRNodepos and viewpos
    //auto d = AMRNode->get_center3() - viewPos;
    float dx = fminf(fabsf(viewPos.x - leaf->lo_.x),fabsf(viewPos.x - leaf->hi_.x));
    float dy = fminf(fabsf(viewPos.z - leaf->lo_.y),fabsf(viewPos.z - leaf->hi_.y));
    //float dz = abs(viewPos.z - viewZ);
    float d = fmaxf(fmaxf(dx, dy), fabsf(viewPos.y - viewY));

    // frustrum culling
    float K = CUTIN_FACTOR*leaf->Size();

    // Subdivision
    if( leaf->level_ < MIN_DEPTH || (leaf->level_ < MAX_DEPTH && d < K) )
    {
        // split and bake heightmap
        leaf->Split(global_model_matrix_);

        Subdivision(viewPos, viewY, (AMRNode*)leaf->child_[0]);
        Subdivision(viewPos, viewY, (AMRNode*)leaf->child_[1]);
        Subdivision(viewPos, viewY, (AMRNode*)leaf->child_[2]);
        Subdivision(viewPos, viewY, (AMRNode*)leaf->child_[3]);

    }
    else
    {
        if( leaf->IsSubdivided() && d >= CUTOUT_FACTOR * K )
        {
            leaf->ReleaseConnectedNodes();
        }
    }
}

void AMRMesh::Draw(Shader& shader) const
{
    shader.setInt("renderType", AMRMesh::RENDER_MODE);
    Draw(this, shader);
}

void AMRMesh::Draw(const AMRNode* leaf, Shader& shader) const
{
    if(leaf->IsSubdivided())
    {
        Draw((AMRNode*)leaf->child_[0], shader);
        Draw((AMRNode*)leaf->child_[1], shader);
        Draw((AMRNode*)leaf->child_[2], shader);
        Draw((AMRNode*)leaf->child_[3], shader);
    }
    else
    {

        // Bind to_render field
        ((AMRNode*)leaf)->BindRenderTarget(&shader, "f0");

        // Transfer local grid model
        shader.setMat4("m4CubeProjMatrix", leaf->model_);

        // Transfer lo and hi
        shader.setInt("level",leaf->level_);
        shader.setInt("hash",leaf->morton_);

        // Render grid (inline function call renderGrid())
        this->AMRNode::Draw();

        //std::cout << "ok" << std::endl;
    }
    return;
}

// static variables
uint AMRMesh::MIN_DEPTH = 0;
uint AMRMesh::MAX_DEPTH = 15;
float AMRMesh::CUTIN_FACTOR = 2.0f; // 2.8 -> see function definition
float AMRMesh::CUTOUT_FACTOR = 1.0f; // >= 1
bool AMRMesh::FRUSTRUM_CULLING = false;
bool AMRMesh::CRACK_FILLING = false;
RenderMode AMRMesh::RENDER_MODE = REAL;

#include "imgui.h"

void AMRMesh::GuiInterface()
{
    static int counter = 0;

    //ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("AMRMesh::Control Panel"))
    {
        ImGui::Text("Controllable parameters for AMRMesh class.");
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
            // Use SetNextItemOpen() so set the default state of a AMRNode to be open.
            // We could also use TreeAMRNodeEx() with the ImGuiTreeAMRNodeFlags_DefaultOpen flag to achieve the same thing!
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
