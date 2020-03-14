#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 projection_view;
uniform mat4 model;
void main()
{
    TexCoords = aTexCoords;
    FragPos = vec3(model * vec4(aPos, 1.0f));

    gl_Position = projection_view * model *vec4(aPos, 1.0);
}
