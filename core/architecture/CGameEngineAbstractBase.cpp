#include "CGameEngineAbstractBase.h"

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "camera.h"

CGameEngineAbstractBase::CGameEngineAbstractBase():
    m_nWidth(1600),
    m_nHeight(900),
    m_fCurrentTime(0),
    m_fLastTime(0),
    m_fDeltaTime(0),
    m_fLastX(m_nWidth / 2.0f),
    m_fLastY(m_nHeight/ 2.0f),
    m_bFirstMouse(true),
    m_bMouseButtonRight(false),
    m_pControllerCamera(NULL)
{
    m_pControllerCamera = new Camera(0,0,0,0,1,0,GetFrameRatio());
}

CGameEngineAbstractBase::~CGameEngineAbstractBase()
{
    if(m_pControllerCamera)
        delete m_pControllerCamera;
}

void CGameEngineAbstractBase::Tick(float dt)
{
    AdvanceTime(dt);
    Update();
    RenderUpdate();
}

static int key_space_old_state = GLFW_RELEASE;
void CGameEngineAbstractBase::ProcessInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        GetCurrentCamera()->ProcessKeyboard(FORWARD, GetDeltaTime());
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        GetCurrentCamera()->ProcessKeyboard(BACKWARD, GetDeltaTime());
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        GetCurrentCamera()->ProcessKeyboard(LEFT, GetDeltaTime());
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        GetCurrentCamera()->ProcessKeyboard(RIGHT, GetDeltaTime());

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        GetCurrentCamera()->ProcessKeyboard(COUNTERCLOCKWISE, GetDeltaTime());
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        GetCurrentCamera()->ProcessKeyboard(CLOCKWISE, GetDeltaTime());

    if ((glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) && (key_space_old_state == GLFW_RELEASE))
    {
        GetCurrentCamera()->printInfo();
    }
    key_space_old_state = glfwGetKey(window, GLFW_KEY_SPACE);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void CGameEngineAbstractBase::FramebufferSizeCallback(GLFWwindow* window, int w, int h)
{
    SetFrameWidthAndHeight(w,h);
    GetCurrentCamera()->updateAspect(GetFrameRatio());
    // if hdr, go and update size of texture
}

void CGameEngineAbstractBase::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if(!window) return;
    if (m_bFirstMouse)
    {
        m_fLastX = float(xpos);
        m_fLastY = float(ypos);
        m_bFirstMouse = false;
    }

    float xoffset = float(xpos) - m_fLastX;
    float yoffset = m_fLastY - float(ypos); // reversed since y-coordinates go from bottom to top

    m_fLastX = float(xpos);
    m_fLastY = float(ypos);

    if(m_bMouseButtonRight)
        GetCurrentCamera()->ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void CGameEngineAbstractBase::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    GetCurrentCamera()->ProcessMouseScroll(float(yoffset));
}

void CGameEngineAbstractBase::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if(!window) return;
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {m_bMouseButtonRight = true;return;}
    m_bMouseButtonRight = false;
    mods = 0;
}
