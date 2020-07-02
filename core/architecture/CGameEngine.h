#ifndef CGAMEENGINE_H
#define CGAMEENGINE_H

#include "CGameEngineAbstractBase.h"

class Camera;

// initialize openGL
// open a window
// set bindings
class CGameEngine : public CGameEngineAbstractBase
{
public:
    CGameEngine();
    virtual ~CGameEngine();
    virtual void Update();
    virtual void RenderUpdate();
    //virtual Camera* GetCurrentCamera();

};

#endif // CGAMEENGINE_H
