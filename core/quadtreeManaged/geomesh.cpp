#include "geomesh.h"

// Caution: only return subdivided grids.
// write additional condition if you need root
Node* Geomesh::queryNode( const glm::vec2& pos) const
{
    //bool isTraversible = node->subdivided;
        // find the node
        Node* sh_node = root.get();
        int result = -1;
        while(true)
        {
            // start from root
            result = sh_node->search(pos);
            if(result != -1 && sh_node->subdivided)
            {
                sh_node = sh_node->child[result];
                continue;
            }
            break;
        }

        if(result == -1) { return NULL; }
        return sh_node;
}

void Geomesh::refresh_heightmap(Node* node)
{
    //bool isTraversible = node->subdivided;
    if(node->subdivided)
    {
        refresh_heightmap(node->child[0]);
        refresh_heightmap(node->child[1]);
        refresh_heightmap(node->child[2]);
        refresh_heightmap(node->child[3]);
    }
    else
    {
        // Refresh old height map for interface cells
        if(node->crackfixed)
        {
            node->bake_height_map();
        }
    }
}

void Geomesh::fixcrack(Node* node)
{
    //bool isTraversible = node->subdivided;
    if(node->subdivided)
    {
        fixcrack(node->child[0]);
        fixcrack(node->child[1]);
        fixcrack(node->child[2]);
        fixcrack(node->child[3]);
    }
    else
    {
        // fix crack
        // for each block, we only need to check 2 directions
        // because the other 2 directions are filled by same or higher level lod
        // once found a match, the height map of current block will be modified
        // only perform this to current draw level

        //if(node -> crackfixed == true) { return; }

        // marks: 0: left, 1: top, 2:right, 3:bottom

        glm::vec2 f1, f2;
        int e1, e2;
        if(node->offset_type == 0) // going to search 2 faces
        {
            f1 = node->get_center() - glm::vec2(0.0,(node->hi.y-node->lo.y)); // bottom
            f2 = node->get_center() - glm::vec2((node->hi.x-node->lo.x), 0.0); // left
            e1 = 3;e2 = 0;
        }else
        if(node->offset_type == 1) // going to search 2 faces
        {
            f1 = node->get_center() + glm::vec2(0.0,(node->hi.y-node->lo.y)); // top
            f2 = node->get_center() - glm::vec2((node->hi.x-node->lo.x), 0.0); // left
            e1 = 1;e2 = 0;
        }else
        if(node->offset_type == 2) // going to search 2 faces
        {
            f1 = node->get_center() + glm::vec2(0.0,(node->hi.y-node->lo.y)); // top
            f2 = node->get_center() + glm::vec2((node->hi.x-node->lo.x), 0.0); // right
            e1 = 1;e2 = 2;
        }
        else
        if(node->offset_type == 3) // going to search 2 faces
        {
            f1 = node->get_center() - glm::vec2(0.0,(node->hi.y-node->lo.y)); // bottom
            f2 = node->get_center() + glm::vec2((node->hi.x-node->lo.x), 0.0); // right
            e1 = 3;e2 = 2;
        }
        else {return;} // may located at other blocks or out of the bound

        // find the node
        Node* sh_node = queryNode(f1);

        // Compare with neighbour, if my_level > neighbour_level
        // a height map sync will be called here
        if(sh_node && node->level == (sh_node->level + 1))
        {
            node-> fix_heightmap(sh_node,e1);
        }

        // here is the second face
        sh_node = queryNode(f2);

        if(sh_node && node->level == (sh_node->level + 1))
        {
            node-> fix_heightmap(sh_node,e2);
        }
    }
}

void Geomesh::subdivision(const glm::vec3& viewPos, const glm::vec3& viewFront, Node* node)
{

    // distance between nodepos and viewpos
    float d = glm::distance(viewPos, node->get_center3());

    // frustrum culling
    float K = CUTIN_FACTOR*node->size();
    if(FRUSTRUM_CULLING)
    {
        K = glm::dot(viewFront, node->get_center3() - viewPos) > 0.0f || d < node->size() ? K:0.0f;
    }

    // Subdivision
    if( d < K && node->level < MAX_DEPTH )
    {

        // split and bake heightmap
        node->split();

        subdivision(viewPos, viewFront, node->child[0]);
        subdivision(viewPos, viewFront, node->child[1]);
        subdivision(viewPos, viewFront, node->child[2]);
        subdivision(viewPos, viewFront, node->child[3]);

    }
    else
    {
        if( (node->level >= MAX_DEPTH || d > (CUTOUT_FACTOR * K)) && node->subdivided )
        {
            delete node->child[0];
            delete node->child[1];
            delete node->child[2];
            delete node->child[3];

            node->subdivided = false;
        }
    }
}

void Geomesh::drawRecr(Node* node, Shader& shader) const
{
    if(node->subdivided)
    {
        drawRecr(node->child[0], shader);
        drawRecr(node->child[1], shader);
        drawRecr(node->child[2], shader);
        drawRecr(node->child[3], shader);
    }
    else
    {
        // Transfer local grid model
        //glm::mat4 model(1);

        glm::mat4 _model = glm::translate(glm::mat4(1), node->get_shift());
        _model = glm::scale(_model, node->get_scale());
        shader.setMat4("model", _model);

        // Transfer lo and hi
        //shader.setVec2("lo", node->lo);
        //shader.setVec2("hi", node->hi);

        // Active textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, node->heightmap);

        // Render grid (inline function call renderGrid())
        Node::draw();
    }
    return;
}

// static variables
uint Geomesh::MAX_DEPTH = 9;
float Geomesh::CUTIN_FACTOR = 2.8f; // 2.8 -> see function definition
float Geomesh::CUTOUT_FACTOR = 1.15f; // >= 1
bool Geomesh::FRUSTRUM_CULLING = true;
bool Geomesh::CRACK_FILLING = true;
RenderMode Geomesh::RENDER_MODE = REAL;

#include "imgui.h"

void Geomesh::gui_interface()
{
    static int counter = 0;

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::TreeNode("Geomesh::Control Panel"))
    {
        ImGui::Text("Controllable parameters for Geomesh class.");
        ImGui::SliderInt("max depth", (int*)&MAX_DEPTH, 0, 11);
        ImGui::SliderFloat("cutout factor", &CUTOUT_FACTOR, 1.01f, 3.0f);

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
            // Use SetNextItemOpen() so set the default state of a node to be open.
            // We could also use TreeNodeEx() with the ImGuiTreeNodeFlags_DefaultOpen flag to achieve the same thing!
            if (i == 0)
                ImGui::SetNextItemOpen(true, ImGuiCond_Once);

            if (ImGui::TreeNode((void*)(intptr_t)i, "Child %d", i))
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
        // Consider using the lower-level ImDrawList::AddImage() API, via ImGui::GetWindowDrawList()->AddImage().
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
