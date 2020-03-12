#ifndef MESH_H
#define MESH_H
#include <vector>

//GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// Abstruct base
class Mesh
{
public:
    std::vector<unsigned int> m_Polygons;
    std::vector<float> m_Vertices;
    unsigned int VAO, VBO, EBO;

    Mesh(){ glGenVertexArrays(1, &dummyVAO); }
    virtual ~Mesh(){ glDeleteVertexArrays(1, &dummyVAO); }
    virtual void draw() = 0;
    void dummyDraw()
    {
        glBindVertexArray(dummyVAO);
        glDrawArrays(GL_POINTS, 0, 1);
        glBindVertexArray(0);
    }

private:
    unsigned int dummyVAO;
};

#endif
