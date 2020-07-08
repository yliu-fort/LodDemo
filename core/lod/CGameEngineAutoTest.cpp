#include "CGameEngine.h"

#include <iostream>
#include <cmath>
#include <memory>

#include <glad/glad.h>

#include "glm/glm.hpp"

#include "cmake_source_dir.h"
#include "shader.h"
#include "camera.h"
#include "filesystemmonitor.h"
#include "gui_interface.h"
#include "imgui.h"
#include "texture_utility.h"
#include "shape.h"

#include "PNode.h"
#include "AMRMesh.h"
#include "refcamera.h"

#include "UFramebuffer.h"

#include "UCartesianMath.h"

using namespace glm;

void CGameEngine::AutoTest()
{

    float x = min(max(0.7f * 65536.0f, 0.0f), 65535.0f);
    x = 0.0f;
    uint nx = (uint)x;
    uint vx = Umath::ExpandBits1(nx);
    auto rvx = Umath::MergeBits1(vx);
    printf("Encoded morton code %x, %x\n",vx, rvx);

    vec2 pos(0.2,0.76);
    auto v = Umath::EncodeMortonWithLod2(pos,5);
    auto restoredPos = Umath::DecodeMortonWithLod2(v,5);
    printf("Original (%f, %f) Decoded 2D pos (%f, %f)\n",pos.x, pos.y, restoredPos.x, restoredPos.y);

    vec3 pos3(0.2,0.76, 0.34);
    auto v3 = Umath::EncodeMortonWithLod3(pos3,0);
    auto restoredPos3 = Umath::DecodeMortonWithLod3(v3,0);
    printf("Original (%f, %f, %f) Decoded 3D pos (%f, %f, %f)\n",pos3.x, pos3.y, pos3.z, restoredPos3.x, restoredPos3.y, restoredPos3.z);

    v = Umath::EncodeMortonWithLod2(pos,5);
    auto LTpos = Umath::DecodeMortonWithLod2(Umath::GetNeighbourWithLod2(v, -1,  1, 5),5);
    auto RTpos = Umath::DecodeMortonWithLod2(Umath::GetNeighbourWithLod2(v,  1,  1, 5),5);
    auto LBpos = Umath::DecodeMortonWithLod2(Umath::GetNeighbourWithLod2(v, -1, -1, 5),5);
    auto RBpos = Umath::DecodeMortonWithLod2(Umath::GetNeighbourWithLod2(v,  1, -1, 5),5);
    printf("Original (%f, %f) Decoded 2D left     top (%f, %f) dist=%e\n",pos.x, pos.y, LTpos.x, LTpos.y, length(pos-LTpos));
    printf("Original (%f, %f) Decoded 2D right    top (%f, %f) dist=%e\n",pos.x, pos.y, RTpos.x, RTpos.y, length(pos-RTpos));
    printf("Original (%f, %f) Decoded 2D left  bottom (%f, %f) dist=%e\n",pos.x, pos.y, LBpos.x, LBpos.y, length(pos-LBpos));
    printf("Original (%f, %f) Decoded 2D right bottom (%f, %f) dist=%e\n",pos.x, pos.y, RBpos.x, RBpos.y, length(pos-RBpos));

    //exit(EXIT_SUCCESS);
}
