#ifndef CGameEngineAbstractBase_H
#define CGameEngineAbstractBase_H

class GLFWwindow;
class Camera;

// initialize openGL
// open a window
// set bindings
class CGameEngineAbstractBase
{
public:
    CGameEngineAbstractBase();
    virtual ~CGameEngineAbstractBase();
    virtual void ProcessInput( GLFWwindow* );
    virtual void FramebufferSizeCallback( GLFWwindow*, int, int );
    virtual void MouseCallback( GLFWwindow*, double, double );
    virtual void ScrollCallback( GLFWwindow*, double, double );
    virtual void MouseButtonCallback( GLFWwindow*, int, int, int );
    virtual void Tick(float dt);
    virtual void Update() = 0; // physical stuff etc
    virtual void RenderUpdate() = 0; // graphic stuff etc
    virtual Camera* GetCurrentCamera() { return this->m_pControllerCamera; }

    const float& GetCurrentTime() const { return this->m_fCurrentTime; }
    const float& GetLastTime() const { return this->m_fLastTime; }
    const float& GetDeltaTime() const { return this->m_fDeltaTime; }
    int GetFrameWidth() const { return this->m_nWidth; }
    int GetFrameHeight() const { return this->m_nHeight; }
    int& GetFrameWidthRef() { return this->m_nWidth; }
    int& GetFrameHeightRef() { return this->m_nHeight; }
    float GetFrameRatio() const { return float(this->m_nWidth)/float(this->m_nHeight); }

    void SetFrameWidthAndHeight(int w, int h)  { this->m_nWidth = w; this->m_nHeight = h; }

protected:
    void AdvanceTime(const float& dt)
    {
        this->m_fLastTime = this->m_fCurrentTime;
        this->m_fCurrentTime += dt;
        this->m_fDeltaTime = dt;
    }

private:
    int m_nWidth;
    int m_nHeight;
    int m_nSingleStep;

    float m_fCurrentTime;
    float m_fLastTime;
    float m_fDeltaTime;
    float m_fLastX;
    float m_fLastY;

    bool m_bFirstMouse;
    bool m_bMouseButtonRight;
    bool m_bPause;


    Camera* m_pControllerCamera;
};

#endif // CGameEngineAbstractBase_H
