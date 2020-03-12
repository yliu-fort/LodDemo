#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
    //FragColor = texture(tex, TexCoords);
    //FragColor = vec4(TexCoords.x + 1.0f,TexCoords.y,0.0 ,1.0);
}
