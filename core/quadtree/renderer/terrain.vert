#version 330 core
#extension GL_ARB_shading_language_420pack: enable
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 projection_view;
uniform mat4 model;
uniform sampler2D tex0;

uniform vec4 lod_level;

void main()
{
    TexCoords = (lod_level.xz + aTexCoords/pow(2.0f, lod_level.a)) / 4.5;

    vec3 fragPos = aPos;

    //float height = texture(tex0, TexCoords).r;
    //fragPos.y += 0.01f*height;

    // Noise sampler1D
    vec3 density = vec3(0);
    float freq[7] = {4.03,1.96,1.01,1.0f/2.03f,1.0f/3.98f,1.0f/8.01f,1.0f/15.97f};

    for(int i = 0; i < 6; i++)
    {

        density += texture(tex0,  TexCoords * freq[i]).xxx;
        density /= 2.0f;

    }

    fragPos.y += 0.25f*density.x - 0.1f;

    gl_Position = projection_view*model*vec4(fragPos, 1.0);
}
