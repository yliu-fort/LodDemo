#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D tex;

void main()
{
    //FragColor = vec4(1.0);
    FragColor = texture(tex, TexCoords);
    //FragColor = vec4(TexCoords.x,TexCoords.y,0.0 ,1.0);
}
