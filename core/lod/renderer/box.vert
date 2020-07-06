#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 m4ViewProjectionMatrix;
uniform mat4 m4ModelMatrix;
uniform mat4 m4CubeProjMatrix;
void main()
{
    TexCoords = aTexCoords;

    gl_Position = m4ViewProjectionMatrix * m4ModelMatrix * m4CubeProjMatrix * vec4(aPos, 1.0);
}
