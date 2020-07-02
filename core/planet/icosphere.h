#ifndef PRIMITIVES_H
#define PRIMITIVES_H
#include <iostream>
#include <cmath>
#include <map>
#include <vector>

//GLAD
#include <glad/glad.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "mesh.h"

typedef unsigned int uint;
// Primitive classes
// sets of polygons
// VAO, VBO, EBO
// Draw function
class Icosphere : public Mesh
{
    //std::vector<uint> m_Polygons;
    //std::vector<float> m_Vertices;
    std::map<uint, uint> m_SharedVertexDict;
    //uint VAO, VBO, EBO;

public:
    virtual void draw()
    {
        glBindVertexArray(VAO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glDrawElements(GL_TRIANGLES, m_Polygons.size(), GL_UNSIGNED_INT, NULL);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    ~Icosphere()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }

    Icosphere (uint r = 0)
    {
        // compute 12 vertices of icosahedron
        float t = (1.0f + sqrtf (5.0f)) / 2.0f;

        addVertex(-1,  t,  0);
        addVertex( 1,  t,  0);
        addVertex(-1, -t,  0);
        addVertex( 1, -t,  0);

        addVertex( 0, -1,  t);
        addVertex( 0,  1,  t);
        addVertex( 0, -1, -t);
        addVertex( 0,  1, -t);

        addVertex( t,  0, -1);
        addVertex( t,  0,  1);
        addVertex(-t,  0, -1);
        addVertex(-t,  0,  1);

        addIndices( 0, 11,  5);
        addIndices( 0,  5,  1);
        addIndices( 0,  1,  7);
        addIndices( 0,  7, 10);

        addIndices( 0, 10, 11);
        addIndices( 1,  5,  9);
        addIndices( 5, 11,  4);
        addIndices(11, 10,  2);

        addIndices(10,  7,  6);
        addIndices( 7,  1,  8);
        addIndices( 3,  9,  4);
        addIndices( 3,  4,  2);

        addIndices( 3,  2,  6);
        addIndices( 3,  6,  8);
        addIndices( 3,  8,  9);
        addIndices( 4,  9,  5);

        addIndices( 2,  4, 11);
        addIndices( 6,  2, 10);
        addIndices( 8,  6,  7);
        addIndices( 9,  8,  1);

        // Subdivision
        subdivide(r);

        // fix texture coordinates
        fixTexture();

        // Gen buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // fill VBO
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, m_Vertices.size()*sizeof(float), &m_Vertices[0], GL_STATIC_DRAW);
        // fill EBO
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Polygons.size()*sizeof(uint), &m_Polygons[0], GL_STATIC_DRAW);

        // link vertex attributes
        glBindVertexArray(VAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

    }

    void subdivide(uint recursions)
        {

            for (uint i = 0; i < recursions; i++)
            {
                std::map<std::pair<uint,uint>, uint> dict;
                std::vector<uint> new_Polygons;
                for(uint j = 0; j < m_Polygons.size(); j+=3)
                {
                    uint a = m_Polygons[j];
                    uint b = m_Polygons[j+1];
                    uint c = m_Polygons[j+2];

                    uint ab = getMidPointIndex (dict, a, b);
                    uint bc = getMidPointIndex (dict, b, c);
                    uint ca = getMidPointIndex (dict, c, a);

                    addIndices (new_Polygons, a, ab, ca);
                    addIndices (new_Polygons, b, bc, ab);
                    addIndices (new_Polygons, c, ca, bc);
                    addIndices (new_Polygons,ab, bc, ca);
                }
                m_Polygons = new_Polygons;
            }
        }

        uint getMidPointIndex (std::map<std::pair<uint,uint>, uint>& dict, uint indexA, uint indexB)
        {

            uint smallerIndex = indexA < indexB ? indexA : indexB;
            uint greaterIndex = indexA > indexB ? indexA : indexB;

            auto ret = dict.find (std::pair<uint, uint>(smallerIndex, greaterIndex));
            if (ret != dict.end())
                return ret->second;

            float mx = 0.5f*(m_Vertices [8*indexA] + m_Vertices [8*indexB]);
            float my = 0.5f*(m_Vertices [8*indexA+1] + m_Vertices [8*indexB+1]);
            float mz = 0.5f*(m_Vertices [8*indexA+2] + m_Vertices [8*indexB+2]);

            uint next = m_Vertices.size()/8;
            addVertex(mx,my,mz);

            dict.insert (std::pair<std::pair<uint,uint>, uint>(std::pair<uint,uint>(smallerIndex, greaterIndex), next));
            return next;
        }

        void addVertex(float x1, float x2, float x3)
        {
            // normalize
            float norm = sqrtf(x1*x1 + x2*x2 + x3*x3);
            norm = (norm > 0)?norm : 1;

            m_Vertices.push_back (x1/norm);
            m_Vertices.push_back (x2/norm);
            m_Vertices.push_back (x3/norm);

            m_Vertices.push_back (x1/norm);
            m_Vertices.push_back (x2/norm);
            m_Vertices.push_back (x3/norm);

            float s = 0.5f - atan2f(x3,x1) / (2.0f * 3.141592654f);
            float t = 0.5f - (asinf(x2/norm) / 3.141592654f);
            m_Vertices.push_back (s);
            m_Vertices.push_back (t);
        }
        void addIndices(uint i, uint j, uint k)
        {
            m_Polygons.push_back (i);
            m_Polygons.push_back (j);
            m_Polygons.push_back (k);
        }
        void addIndices(std::vector<uint>& indices, uint i, uint j, uint k)
        {
            indices.push_back (i);
            indices.push_back (j);
            indices.push_back (k);
        }

        void fixTexture()
        {
            // scan mesh
            m_SharedVertexDict.clear();
            for(uint i = 0; i < m_Polygons.size(); i += 3)
            {
                uint* a = &m_Polygons[i];
                uint* b = &m_Polygons[i+1];
                uint* c = &m_Polygons[i+2];


                // if find, duplicate vertex with modified s value
                float dist1 = m_Vertices[8*(*a)+6] - m_Vertices[8*(*b)+6];
                if(fabsf(dist1) > 0.5f)
                {
                    if(dist1 > 0) { dupVertexWithModifiedS(b); }
                    else {dupVertexWithModifiedS(a);}
                }

                float dist2 = m_Vertices[8*(*b)+6] - m_Vertices[8*(*c)+6];
                if(fabsf(dist2) > 0.5f)
                {
                    if(dist2 > 0) { dupVertexWithModifiedS(c); }
                    else {dupVertexWithModifiedS(b);}
                }

                float dist3 = m_Vertices[8*(*a)+6] - m_Vertices[8*(*c)+6];
                if(fabsf(dist3) > 0.5f)
                {
                    if(dist3 > 0) { dupVertexWithModifiedS(c); }
                    else {dupVertexWithModifiedS(a);}
                }
            }
        }
        void dupVertexWithModifiedS(uint* i)
        {
            uint old_index = *i;

            auto ret = m_SharedVertexDict.find (old_index);
            // already duplicated -> return
            if (ret != m_SharedVertexDict.end())
            {
                *i = ret->second; // link polygon to new index
                return;
            }
            // not -> add new vertex
            uint new_index = m_Vertices.size()/8;
            addVertex(m_Vertices[8*old_index],m_Vertices[8*old_index+1],m_Vertices[8*old_index+2]);
            m_Vertices[8*new_index+6] += 1; // modify s
            *i = new_index; // link polygon to new index

            // Add into dictionary
            m_SharedVertexDict.insert (std::pair<uint, uint>(old_index, new_index));

            return;
        }

} ;


#endif
