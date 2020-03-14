#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform vec3 albedo;

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = vec4(100.0f*texture(texture_diffuse1,TexCoords).rgb,1.0);
}
