#include "CGameEngine.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "camera.h"

void CGameEngine::Update()
{

}

void CGameEngine::RenderUpdate()
{
    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
