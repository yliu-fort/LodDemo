#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D tex0;

void main()
{
    //FragColor = vec4(1.0);
    FragColor = vec4(texture(tex0, TexCoords).rrr, 1.0f);
}
