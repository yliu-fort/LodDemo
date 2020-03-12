#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 projectionMatrix;
uniform sampler2D tex0;

void main()
{
    TexCoords = aTexCoords;

    vec3 fragPos = aPos;

    float height = texture(tex0, TexCoords).r;
    fragPos.y += height;

    gl_Position = projectionMatrix*vec4(fragPos, 1.0);
}
