#ifndef CGAMEAPP_H
#define CGAMEAPP_H

class CGameEngine;
class GLFWwindow;

class CGameApp
{
// Attributes
protected:
    GLFWwindow* m_pWindow;
    bool m_bActive;
    int m_nWidth, m_nHeight;
    double m_fCurrentTime;

    CGameEngine *m_pGameEngine;

// Operations
protected:
    int OnCreate();
    void OnDestroy();
    void OnSize(GLFWwindow* window, int nWidth, int nHeight);

public:
    CGameApp() : m_bActive(false) {}

    void Run();
    bool InitMode(bool bFullScreen, int nWidth, int nHeight);
    virtual bool InitInstance();
    virtual int ExitInstance();
    virtual bool OnIdle();
    void MakeCurrent();
    void BindCallbacks();
    bool IsActive()								{ return m_bActive; }
    CGameEngine *GetGameEngine()				{ return m_pGameEngine; }
    GLFWwindow *GetWindowHandle()				{ return m_pWindow; }

    int GetWidth()	{ return m_nWidth; }
    int GetHeight()	{ return m_nHeight; }
};

#endif // CGAMEAPP_H
