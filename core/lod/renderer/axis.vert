#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 color;

uniform mat4 m4ViewProjectionMatrix;
uniform mat4 m4ModelMatrix;

void main()
{
    color = aColor;
    gl_Position = m4ViewProjectionMatrix * m4ModelMatrix *vec4(aPos, 1.0);
}
