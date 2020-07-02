#include "CGameApp.h"
#include "CGameEngine.h"

#include <iostream>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "gui_interface.h"

int main(void)
{
    CGameApp app;
    if(app.InitInstance())
        app.Run();
    return app.ExitInstance();
}

bool CGameApp::InitInstance()
{
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        EXIT_FAILURE;
    }

    //glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __linux__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#endif
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return InitMode(false,1600,900);
}

bool CGameApp::InitMode(bool bFullScreen, int nWidth, int nHeight)
{
    m_nWidth = nWidth;
    m_nHeight = nHeight;

    // Open a window and create its OpenGL context
    m_pWindow = glfwCreateWindow( m_nWidth, m_nHeight, "", nullptr, nullptr);
    if( m_pWindow == nullptr ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_pWindow);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // Initial viewport
    glViewport(0, 0, m_nWidth, m_nHeight);

    // Query infomation
    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum nr of vertex attributes supported: " << nrAttributes << std::endl;
    std::cout << "Hardware: " <<glGetString(GL_RENDERER) << std::endl;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &nrAttributes);
    std::cout << "Maximum nr of color attachments supported: " << nrAttributes << std::endl;
    glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &nrAttributes);
    std::cout << "Maximum nr of image uniforms supported by fragment shader: " << nrAttributes << std::endl;
    glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &nrAttributes);
    std::cout << "Maximum nr of image uniforms supported by compute shader: " << nrAttributes << std::endl;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &nrAttributes);
    std::cout << "GL_MAX_SHADER_STORAGE_BLOCK_SIZE is " << nrAttributes << " bytes." << std::endl;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &nrAttributes);
    std::cout << "Maximum nr of shader storage buffer binding points is " << nrAttributes << " ." << std::endl;

    // Compute Shader Configuration
    int work_grp_cnt[3], work_grp_inv;

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

    printf("max global (total) work group size x:%i y:%i z:%i\n",
           work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    printf("max local work group invocations %i\n", work_grp_inv);

    return true;
}

int CGameApp::ExitInstance()
{
    glfwTerminate();
    return 0;
}

void CGameApp::Run()
{
    OnCreate();
    m_fCurrentTime = glfwGetTime();
    while(!glfwWindowShouldClose( m_pWindow ))
    {
        OnIdle();
    }
    OnDestroy();
}

bool CGameApp::OnIdle()
{
    if(m_bActive)
        return false;
    auto deltaT = glfwGetTime() - m_fCurrentTime;
    glfwGetFramebufferSize(m_pWindow, &m_nWidth, &m_nHeight);
    m_pGameEngine->ProcessInput(m_pWindow);
    m_pGameEngine->Tick(deltaT);
    m_fCurrentTime += deltaT;
    glfwSwapBuffers(m_pWindow);
    glfwPollEvents();

    return true;
}

int CGameApp::OnCreate()
{
    if(m_pWindow)
        glfwDestroyWindow(m_pWindow);

    // Open a window and create its OpenGL context
    m_pWindow = glfwCreateWindow( m_nWidth, m_nHeight, "Infinite World", nullptr, nullptr);
    if( m_pWindow == nullptr ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n" );
        getchar();
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_pWindow);

    // Initial viewport
    glViewport(0, 0, m_nWidth, m_nHeight);

    // Initialize game engine
    m_pGameEngine = new CGameEngine;
    BindCallbacks();

    // Mouse input mode
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(0); // 60 fps constraint

    GuiInterface::Init(m_pWindow);

    return 0;
}

void CGameApp::OnDestroy()
{
    if(m_pGameEngine)
    {
        delete m_pGameEngine;
        m_pGameEngine = NULL;
    }
    glfwDestroyWindow(m_pWindow);
}

void CGameApp::OnSize(GLFWwindow* window, int nWidth, int nHeight)
{
    if(!window)
        return;
    if(!nHeight || !nWidth)
        return;
    glViewport(0, 0, nWidth, nHeight);
    m_nWidth = nWidth;
    m_nHeight = nHeight;
    m_pGameEngine->FramebufferSizeCallback(window, nWidth, nHeight);
}

void CGameApp::BindCallbacks()
{
    // set glfw modes
    glfwSetWindowUserPointer(m_pWindow, reinterpret_cast<void *>(this));

    auto FramebufferSizeCallback = [](GLFWwindow* window, int w, int h)
    {
        static_cast<CGameApp*>(glfwGetWindowUserPointer(window))->OnSize( window, w, h );
    };
    auto MouseCallback = [](GLFWwindow* window, double xpos, double ypos)
    {
        static_cast<CGameApp*>(glfwGetWindowUserPointer(window))->m_pGameEngine->MouseCallback( window, xpos, ypos );
    };
    auto ScrollCallback = [](GLFWwindow* window, double xoffset, double yoffset)
    {
        static_cast<CGameApp*>(glfwGetWindowUserPointer(window))->m_pGameEngine->ScrollCallback( window, xoffset, yoffset );
    };
    auto MouseButtonCallback = [](GLFWwindow* window, int button, int action, int mods)
    {
        static_cast<CGameApp*>(glfwGetWindowUserPointer(window))->m_pGameEngine->MouseButtonCallback( window, button, action, mods );
    };

    glfwSetFramebufferSizeCallback(m_pWindow, FramebufferSizeCallback);
    glfwSetCursorPosCallback(m_pWindow, MouseCallback);
    glfwSetScrollCallback(m_pWindow, ScrollCallback);
    glfwSetMouseButtonCallback(m_pWindow, MouseButtonCallback);
}
