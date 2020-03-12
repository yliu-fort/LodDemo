#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 albedo;
uniform float globalScaling;

void main()
{
    vec3 scaledFragPos = FragPos * globalScaling;

    vec3 color = vec3(1.0f);
    float sizeOfShadow = 64.0f;
    if(mod(scaledFragPos.x, sizeOfShadow) < sizeOfShadow/2 || mod(scaledFragPos.z, sizeOfShadow) < sizeOfShadow/2)
    {
        color = vec3(0.0f);
    }
    FragColor = vec4(mix(color, albedo, 0.25f), 1.0f);
}
