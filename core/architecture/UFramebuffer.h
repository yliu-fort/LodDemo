#pragma once

// a simple wrapper class
class UFrameBuffer
{
public:
    UFrameBuffer(){}
    UFrameBuffer(int w, int h){ Alloc(w,h); }
    virtual ~UFrameBuffer(){ Release(); }
    void Resize( int, int );
    void Release();
    const unsigned int& GetFBO() const { return frameBufferObject; }
    const unsigned int& GetCBO() const { return colorBufferObject; }
    const unsigned int& GetRBO() const { return depthBufferObject; }

    virtual void BindAndClear();

protected:
    bool Alloc( int, int );

protected:
    unsigned int frameBufferObject, colorBufferObject, depthBufferObject;
    int nWidth, nHeight;
};


// according to the reference w and h
// this framebuffer will adjust its size automatically
// this class is not allowed to modify external pointer
class UFrameBufferAutoAdjusted : protected UFrameBuffer
{
public:
    UFrameBufferAutoAdjusted() : UFrameBuffer(){}
    UFrameBufferAutoAdjusted(const int *w, const int *h) : UFrameBuffer(*w,*h), m_pWidth(w), m_pHeight(h){ }
    ~UFrameBufferAutoAdjusted(){ }
    const unsigned int& GetFBO() const { return frameBufferObject; }
    const unsigned int& GetCBO() const { return colorBufferObject; }
    const unsigned int& GetRBO() const { return depthBufferObject; }

    virtual void BindAndClear() override;
    void BindSizeReferences(const int *w, const int *h) { m_pWidth = w; m_pHeight = h; }

protected:
    const int *m_pWidth, *m_pHeight;
};
