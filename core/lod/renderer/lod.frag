#version 330 core
#extension GL_ARB_shading_language_420pack: enable
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;

uniform vec3 albedo;
uniform float globalScaling;
uniform float localScaling;
uniform sampler2D tex0;

void main()
{
    vec2 scaledTexCoords = (TexCoords*2 - 1) * (globalScaling/4096.0f) * localScaling; // create symmetric indexing
    vec3 scaledFragPos = FragPos * globalScaling;

    vec3 color = vec3(1.0f);
    float sizeOfShadow = 64.0f;
    if(mod(scaledFragPos.x, sizeOfShadow) < sizeOfShadow/2 || mod(scaledFragPos.z, sizeOfShadow) < sizeOfShadow/2)
    {
        color = vec3(0.0f);
    }

    // Noise sampler1D
    vec3 density = vec3(0);
    float freq[7] = {4.03,1.96/2,1.01/4,1.0f/2.03f/8,1.0f/3.98f/16,1.0f/8.01f/32,1.0f/15.97f/64};
    float scaling[7] = {1.0, 64.0, 256.0, 1024.0, 4096.0, 16384.0, 65536.0};

    for(int i = 0; i < 7; i++)
    {

        density = mix(density, (0.25f*density.yzx + texture(tex0,  scaledTexCoords * freq[i]).xyz)/1.25f, min(globalScaling/scaling[i],1.0f));
        //density = 0.5f*density.yzx + texture(tex0,  scaledTexCoords * freq[i]).xyz;
        if(globalScaling < scaling[i]) { break; }

    }

    FragColor = vec4(mix(color, vec3(density.rrr), 0.99f), 1.0f);

}
