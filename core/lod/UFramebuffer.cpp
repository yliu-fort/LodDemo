#include "UFramebuffer.h"

#include <iostream>
#include "glad/glad.h"

bool UFrameBuffer::Alloc(int w, int h)
{
    nWidth = w;
    nHeight = h;
    glGenFramebuffers(1, &frameBufferObject);
    // create floating point color buffer
    glGenTextures(1, &colorBufferObject);
    glBindTexture(GL_TEXTURE_2D, colorBufferObject);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // create depth buffer (renderbuffer)
    glGenRenderbuffers(1, &depthBufferObject);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferObject);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferObject, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferObject);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void UFrameBuffer::Release()
{
    if(colorBufferObject)
        glDeleteTextures(1,&colorBufferObject);
    if(depthBufferObject)
        glDeleteRenderbuffers(1,&depthBufferObject);
    if(frameBufferObject)
        glDeleteFramebuffers(1,&frameBufferObject);
}

void UFrameBuffer::Resize(int w, int h)
{
    if(w == nWidth && h == nHeight)
        return;
    Release();
    Alloc(w, h);
}

void UFrameBuffer::BindAndClear()
{
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,nWidth, nHeight);
}



void UFrameBufferAutoAdjusted::BindAndClear()
{
    // auto-adjust
    Resize(*m_pWidth, *m_pHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferObject);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0,0,nWidth, nHeight);
}
