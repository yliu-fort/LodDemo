#include "AMRMesh.h"
#include <glad/glad.h>
#include "UCartesianMath.h"
#include <array>


void AMRMesh::MultiLevelIntegrator()
{
    UpdateLevelOrderList();

    Shader* advector = &(AMRNode::kShaderList.at("advector"));
    Shader* patch_communicator = &(AMRNode::kShaderList.at("communicator"));
    Shader* patch_interpolater = &(AMRNode::kShaderList.at("interpolater"));
    Shader* patch_extrapolator = &(AMRNode::kShaderList.at("extrapolator"));

    constexpr std::array<glm::vec2, 8> wi({glm::vec2(-1, 0),
                                           glm::vec2( 1, 0),
                                           glm::vec2( 0,-1),
                                           glm::vec2( 0, 1),
                                           glm::vec2(-1,-1),
                                           glm::vec2(-1, 1),
                                           glm::vec2( 1,-1),
                                           glm::vec2( 1, 1)
                                          });

    auto L_vFrontPropagation = [&](const std::vector<std::vector<PNode*>>& list, int lod)
    {


        // Nothing to do with the finest level
        if(list[lod] == list.back()) { return; }
        //// Extrapolates field from fine meshes to coarse meshes
        patch_extrapolator->use();
        for(auto idx: wi) // ! Must be outer loop or cannot guarantee the edge - corner order
        {
            int i = idx.x, j = idx.y;
            if(i == 0 && j == 0) continue;

            for(auto block: list[lod])
            {
                // Nothing to do with block covered by finer meshes
                if(block->IsSubdivided()) continue;

                auto buffer_ = ((AMRNode*)block)->GetFieldPtr();

                // Interface only appears in certain directions
                //if(!block->ContainToCoarseInterface(i,j)) continue;

                // Get neighbour node handle
                auto shared_node = QueryNode(
                            Umath::GetNeighbourWithLod2(block->morton_, i,  j, 15-block->level_)
                            );

                // Block boundary patches
                if(shared_node == nullptr) continue;

                // Finds coarse to fine interface (implication: lod level diff <= 1)
                if(!shared_node->IsSubdivided()) continue;

                // Iterates all childs mounted in shared node
                for(auto child: shared_node->child_)
                {
                    // Detects chlids attach to the fine interface
                    if(!child->ContainToCoarseInterface(-i,-j)) continue;

                    // now deal with block communication on coarse-fine boundaries.
                    // ...
                    // bind buffers
                    for(auto& id_: *(((AMRNode*)child)->GetFieldPtr())) { id_.second.BindImageFront(); }
                    for(auto& id_: *buffer_) { id_.second.BindTextureFront(); }

                    // set constants ...
                    patch_extrapolator->setInt("xoffset",i);
                    patch_extrapolator->setInt("yoffset",j);
                    patch_extrapolator->setInt("roffset",child->GetOffsetType());

                    glm::uvec2 grid(AMRNode::FIELD_MAP_X/2,AMRNode::FIELD_MAP_Y/2);
                    if(i != 0) { grid.x = 1; }
                    if(j != 0) { grid.y = 1; }

                    // Dispatch kernel
                    glDispatchCompute(grid.x, grid.y, 1);
                    //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                }
            }
        }
    };

    auto L_vAdvection = [&](const std::vector<std::vector<PNode*>>& list, int lod)
    {

        //// Fill External Boundary
        // Periodic
        // 1. find out all the "at most" cells i.e. leftmost, rightmost, within specific lod level
        // 2. find its "mirror", i.e. horizontal mirror, vertical mirror and diagonal mirror
        // 3. copy the relevent ghost layer content
        patch_communicator->use();
        for(auto idx: wi)
        {
            int i = idx.x, j = idx.y;
            if(i == 0 && j == 0) continue;

            for(auto block: list[lod])
            {
                // Get neighbour node handle
                auto shared_code = Umath::GetNeighbourWithLod2(block->morton_, i,  j, 15-block->level_);

                // Deal with the boundary (temporary solution, force periodicity)
                if(shared_code == 0xFFFFFFFFu) // touch the bounding box
                {
                    auto buffer_ = ((AMRNode*)block)->GetFieldPtr();
                    if(i != 0)
                    {
                        auto shared_node = QueryNode( Umath::FlipLRWithLod2(block->morton_, 15-block->level_) );

                        // Block boundary patches and To coarse interface
                        if(shared_node != nullptr && !shared_node->IsSubdivided())
                        {
                            // Block To fine interface

                            // now deal with block communication between "same lod level".
                            // ...
                            // bind buffers
                            for(auto& id_: *buffer_) { id_.second.BindImageFront(); }
                            for(auto& id_: *(((AMRNode*)shared_node)->GetFieldPtr())) { id_.second.BindTextureFront(); }

                            // set constants ...
                            patch_communicator->setInt("xoffset",i);
                            patch_communicator->setInt("yoffset",j);

                            glm::uvec2 grid(AMRNode::FIELD_MAP_X,AMRNode::FIELD_MAP_Y);
                            if(i != 0) { grid.x = 1;}
                            if(j != 0) { grid.y = 1;}

                            // Dispatch kernel
                            glDispatchCompute(grid.x, grid.y, 1);
                        }
                    }
                    if(j != 0)
                    {
                        auto shared_node = QueryNode( Umath::FlipUDWithLod2(block->morton_, 15-block->level_) );

                        // Block boundary patches and To coarse interface
                        if(shared_node != nullptr && !shared_node->IsSubdivided())
                        {
                            // Block To fine interface

                            // now deal with block communication between "same lod level".
                            // ...
                            // bind buffers
                            for(auto& id_: *buffer_) { id_.second.BindImageFront(); }
                            for(auto& id_: *(((AMRNode*)shared_node)->GetFieldPtr())) { id_.second.BindTextureFront(); }

                            // set constants ...
                            patch_communicator->setInt("xoffset",i);
                            patch_communicator->setInt("yoffset",j);

                            glm::uvec2 grid(AMRNode::FIELD_MAP_X,AMRNode::FIELD_MAP_Y);
                            if(i != 0) { grid.x = 1;}
                            if(j != 0) { grid.y = 1;}

                            // Dispatch kernel
                            glDispatchCompute(grid.x, grid.y, 1);
                        }
                    }
                    if(i != 0 && j != 0)
                    {
                        if(Umath::GetNeighbourWithLod2(block->morton_, i,  0, 15-block->level_) == 0xFFFFFFFF
                        && Umath::GetNeighbourWithLod2(block->morton_, 0,  j, 15-block->level_) == 0xFFFFFFFF)
                        {
                            auto shared_node = QueryNode( Umath::FlipDiagWithLod2(block->morton_, 15-block->level_) );

                            // Block boundary patches and To coarse interface
                            if(shared_node != nullptr && !shared_node->IsSubdivided())
                            {
                                // Block To fine interface

                                // now deal with block communication between "same lod level".
                                // ...
                                // bind buffers
                                for(auto& id_: *buffer_) { id_.second.BindImageFront(); }
                                for(auto& id_: *(((AMRNode*)shared_node)->GetFieldPtr())) { id_.second.BindTextureFront(); }

                                // set constants ...
                                patch_communicator->setInt("xoffset",i);
                                patch_communicator->setInt("yoffset",j);

                                glm::uvec2 grid(AMRNode::FIELD_MAP_X,AMRNode::FIELD_MAP_Y);
                                if(i != 0) { grid.x = 1;}
                                if(j != 0) { grid.y = 1;}

                                // Dispatch kernel
                                glDispatchCompute(grid.x, grid.y, 1);
                            }
                        }
                    }
                }
            }
        }

        //// Communicate with other neighbours...
        // fill the boundary patches
        // edge first, corner second
        patch_communicator->use();
        for(auto idx: wi)
        {
            int i = idx.x, j = idx.y;
            if(i == 0 && j == 0) continue;

            for(auto block: list[lod])
            {
                auto buffer_ = ((AMRNode*)block)->GetFieldPtr();

                // Get neighbour node handle
                auto shared_code = Umath::GetNeighbourWithLod2(block->morton_, i,  j, 15-block->level_);
                auto shared_node = QueryNode( shared_code );

                // Block boundary patches and To coarse interface
                if(shared_node == nullptr) continue;

                // Block To fine interface
                if(shared_node->IsSubdivided()) continue;

                // now deal with block communication between "same lod level".
                // ...
                // bind buffers
                for(auto& id_: *buffer_) { id_.second.BindImageFront(); }
                for(auto& id_: *(((AMRNode*)shared_node)->GetFieldPtr())) { id_.second.BindTextureFront(); }

                // set constants ...
                patch_communicator->setInt("xoffset",i);
                patch_communicator->setInt("yoffset",j);

                glm::uvec2 grid(AMRNode::FIELD_MAP_X,AMRNode::FIELD_MAP_Y);
                if(i != 0) { grid.x = 1;}
                if(j != 0) { grid.y = 1;}

                // Dispatch kernel
                glDispatchCompute(grid.x, grid.y, 1);
            }
        }


        //// Interpolates field from fine meshes to coarse meshes
        patch_interpolater->use();
        for(auto idx: wi)
        {
            int i = idx.x, j = idx.y;
            if(i == 0 && j == 0) continue;

            for(auto block: list[lod])
            {
                // Nothing to do with root level
                if(block->level_ == 0) continue;

                auto buffer_ = ((AMRNode*)block)->GetFieldPtr();

                // Interface only appears in certain directions
                if(!block->ContainToCoarseInterface(i,j)) continue;

                // Get neighbour node handle
                auto shared_node = QueryNode(
                            Umath::GetNeighbourWithLod2(block->parent_->morton_, i,  j, 15-block->parent_->level_)
                            );

                // Block boundary patches
                if(shared_node == nullptr) continue;

                // Block To fine interface
                if(shared_node->IsSubdivided()) continue;

                // now deal with block communication on fine-coarse boundaries.
                // ...
                // bind buffers
                for(auto& id_: *(((AMRNode*)shared_node)->GetFieldPtr())) { id_.second.BindImageFront(); } //dest
                for(auto& id_: *buffer_) { id_.second.BindTextureFront(); } // src

                // set constants ...
                patch_interpolater->setInt("xoffset",i);
                patch_interpolater->setInt("yoffset",j);
                patch_interpolater->setInt("roffset",block->GetOffsetType());

                glm::uvec2 grid(AMRNode::FIELD_MAP_X/2,AMRNode::FIELD_MAP_Y/2);
                if(i != 0) { grid.x = 1;}
                if(j != 0) { grid.y = 1;}

                // Dispatch kernel
                glDispatchCompute(grid.x, grid.y, 1);
            }
        }

        //// Advect local field
        advector->use();
        for(auto block: list[lod])
        {
            // bind buffers
            auto buffer_ = ((AMRNode*)block)->GetFieldPtr();
            for(auto& id_: *buffer_)
            {
                id_.second.BindDefault();
                id_.second.SwapBuffer();
            }

            // set constants ...

            // Dispatch kernel
            glDispatchCompute((AMRNode::FIELD_MAP_X/16)+1,(AMRNode::FIELD_MAP_Y/16)+1,1);
        }


    };

    AMRNode::MultiLevelIntegrator(
                //[](const std::vector<std::vector<PNode*>>&, int){},
                L_vFrontPropagation,
                //[](const std::vector<std::vector<PNode*>>&, int){},
                L_vAdvection,
                level_order_list_, 0);
}


// Caution: only return subdivided grids.
// write additional condition if you need root
PNode* AMRMesh::QueryNode( const glm::vec2& pos) const
{
    // find the AMRNode
    auto shared_node = GetHandle();
    int result = -1;
    while(true)
    {
        // start from root
        result = shared_node->Search(pos);
        if(result != -1 && shared_node->IsSubdivided())
        {
            shared_node = shared_node->child_[result];
            continue;
        }
        break;
    }

    if(result == -1) { return NULL; }
    return shared_node;
}

PNode* AMRMesh::QueryNode( uint v ) const
{
    if(v == 0xFFFFFFFFu) return nullptr;
    int lod = Umath::GetLodLevel(v); // 0 : finest, 15: top

    // find the AMRNode
    auto shared_node = GetHandle();

    for(int i = 15-lod; i > 0; --i)
    {
        // start from root
        if(!shared_node->IsSubdivided())
            return nullptr;

        shared_node = shared_node->child_[((v >> (2*i-2)) & 0x3)];
    }

    return shared_node;
}

void AMRMesh::Subdivision(uint code, uint lod)
{
    auto uv = 2.0f*Umath::DecodeMorton2(code)-1.0f;
    auto viewPos = uv;
    auto viewY = 4.0f/(1<<lod);

    auto func = [&, this](PNode* leaf){
        // distance between nodepos and viewpos
        float dx = fminf(fabsf(viewPos.x - leaf->lo_.x),fabsf(viewPos.x - leaf->hi_.x));
        float dy = fminf(fabsf(viewPos.y - leaf->lo_.y),fabsf(viewPos.y - leaf->hi_.y));
        float d = fmaxf(fmaxf(dx, dy), fabsf(viewY));

        // compute refinement factor
        float K = CUTIN_FACTOR*leaf->Size();

        // Subdivision
        if( leaf->level_ < MIN_DEPTH || (leaf->level_ < MAX_DEPTH && d < K) )
        {
            if(!leaf->IsSubdivided())
            {
                // split and bake heightmap
                this->modified_ = true;
                leaf->Split(global_model_matrix_);
            }
            return true;
        }
        else
        {
            if( leaf->IsSubdivided() && d >= CUTOUT_FACTOR * K )
            {
                this->modified_ = true;
                leaf->ReleaseConnectedNodes();
            }
        }
        return false;

    };

    GetHandle()->Exec(func);
}

void AMRMesh::CircularSubdivision(uint code, uint lod)
{

    auto uv = 2.0f*Umath::DecodeMorton2(code)-1.0f;
    auto viewPos_ = uv;
    auto viewY_ = 4.0f/(1<<lod);

    constexpr std::array<glm::vec3, 8> wi({
                                              glm::vec3(-2, 0, 0),
                                              glm::vec3( 2, 0, 0),
                                              glm::vec3( 0,-2, 0),
                                              glm::vec3( 0, 2, 0),
                                              glm::vec3(-2,-2, 0),
                                              glm::vec3(-2, 2, 0),
                                              glm::vec3( 2,-2, 0),
                                              glm::vec3( 2, 2, 0)
                                          });

    auto func = [&, this](PNode* leaf){
        // distance between nodepos and viewpos
        float dx = fminf(fabsf(viewPos_.x - leaf->lo_.x),fabsf(viewPos_.x - leaf->hi_.x));
        float dy = fminf(fabsf(viewPos_.y - leaf->lo_.y),fabsf(viewPos_.y - leaf->hi_.y));
        float d = fmaxf(fmaxf(dx, dy), fabsf(viewY_));

        for(auto dir: wi)
        {
            float dx = fminf(fabsf(viewPos_.x+dir.x - leaf->lo_.x),fabsf(viewPos_.x+dir.x - leaf->hi_.x));
            float dy = fminf(fabsf(viewPos_.y+dir.y - leaf->lo_.y),fabsf(viewPos_.y+dir.y - leaf->hi_.y));
            float di = fmaxf(fmaxf(dx, dy), fabsf(viewY_+dir.z));
            d = fminf(d, di);
        }

        // compute refinement factor
        float K = CUTIN_FACTOR*leaf->Size();

        // Subdivision
        if( leaf->level_ < MIN_DEPTH || (leaf->level_ < MAX_DEPTH && d < K) )
        {
            if(!leaf->IsSubdivided())
            {
                // split and bake heightmap
                this->modified_ = true;
                leaf->Split(global_model_matrix_);
            }
            return true;
        }
        else
        {
            if( leaf->IsSubdivided() && d >= CUTOUT_FACTOR * K )
            {
                this->modified_ = true;
                leaf->ReleaseConnectedNodes();
            }
        }
        return false;

    };


    GetHandle()->Exec(func);
}

void AMRMesh::Draw(Shader& shader) const
{
    shader.setInt("renderType", AMRMesh::RENDER_MODE);

    auto func = [&](PNode* leaf){

        if(leaf->IsSubdivided())
            return true;

        {
            // Bind to_render field
            ((AMRNode*)leaf)->BindRenderTarget("f0");

            // Transfer local grid model
            shader.setMat4("m4CubeProjMatrix", leaf->model_);

            // Transfer lo and hi
            shader.setInt("level",leaf->level_);
            shader.setInt("hash",leaf->morton_);

            // Render grid (inline function call renderGrid())
            ((AMRNode*)leaf)->Draw();

            //std::cout << "ok" << std::endl;
        }
        return false;
    };

    GetHandle()->Exec(func);
}

// static variables
uint AMRMesh::MIN_DEPTH = 0;
uint AMRMesh::MAX_DEPTH = 15;
float AMRMesh::CUTIN_FACTOR = 2.0f; // 2.8 -> see function definition
float AMRMesh::CUTOUT_FACTOR = 1.0f; // >= 1
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
        ImGui::SliderFloat("cutin factor", &CUTIN_FACTOR, 1.0f, 3.0f);
        ImGui::SliderFloat("cutout factor", &CUTOUT_FACTOR, 1.0f, 3.0f);

        // render mode
        const char* RENDER_TYPE_NAMES[ELEMENT_COUNT] = { "REAL", "CONTOUR", "NORMAL", "PCOLOR" };

        const char* current_element_name = (RENDER_MODE >= 0 && RENDER_MODE < ELEMENT_COUNT) ? RENDER_TYPE_NAMES[RENDER_MODE] : "Unknown";
        ImGui::SliderInt("render type", (int*)&RENDER_MODE, 0, ELEMENT_COUNT - 1, current_element_name);

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
