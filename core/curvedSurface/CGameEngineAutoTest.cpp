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
#include "UCartesianMath.h"

using namespace glm;

void CGameEngine::AutoTest()
{

    //float x = min(max(0.7f * 65536.0f, 0.0f), 65535.0f);
    //x = 0.0f;
    //uint nx = (uint)x;
    //uint vx = Umath::ExpandBits1(nx);
    //auto rvx = Umath::MergeBits1(vx);
    //printf("Encoded morton code %x, %x\n",vx, rvx);
    //
    //vec2 pos(0.2,0.76);
    //auto v = Umath::EncodeMortonWithLod2(pos,5);
    //auto restoredPos = Umath::DecodeMortonWithLod2(v,5);
    //printf("Original (%f, %f) Decoded 2D pos (%f, %f)\n",pos.x, pos.y, restoredPos.x, restoredPos.y);
    //
    //vec3 pos3(0.2,0.76, 0.34);
    //auto v3 = Umath::EncodeMortonWithLod3(pos3,0);
    //auto restoredPos3 = Umath::DecodeMortonWithLod3(v3,0);
    //printf("Original (%f, %f, %f) Decoded 3D pos (%f, %f, %f)\n",pos3.x, pos3.y, pos3.z, restoredPos3.x, restoredPos3.y, restoredPos3.z);

    //exit(EXIT_SUCCESS);
}
