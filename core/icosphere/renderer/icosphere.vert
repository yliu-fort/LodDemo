#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 projectionMatrix;

void main()
{
    TexCoords = aTexCoords;
    //vs_out.TexCoords.x = 0.5f - (atan(aPos.z,aPos.x) / (2.0f * 3.141592654f));
    //vs_out.TexCoords.y = 0.5f - (asin(aPos.y) / 3.141592654f);

    gl_Position = projectionMatrix*vec4(aPos, 1.0);
}
