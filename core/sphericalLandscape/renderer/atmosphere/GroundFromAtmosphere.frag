#version 330 core
//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

out vec4 color;

in vec3 v3FrontColor; // InScatter
in vec3 v3FrontSecondaryColor; // Transmittence
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

in float blendNearFar;

uniform sampler2D s2Tex1;               // diffusive - 2
uniform sampler2D s2Tex2;               // specular - 3

uniform vec3 v3CameraPos;		// The camera's current position
uniform vec3 v3LightDir;		// The direction vector to the light source

uniform int level;
uniform int hash;
uniform int renderType;

vec2 getRello(int code)
{

    code >>= (2*(level-1));
    return 0.5f*vec2((code>>1)&1, (code)&1);
}

void main ()
{
    vec3 albedo = 0.1*mix(texture( s2Tex1, TexCoords ).rgb,
                texture( s2Tex2, getRello(hash)+TexCoords/(1.0f + float(level > 0)) ).rgb,
                blendNearFar);

    if(renderType == 1)
    {
        albedo = vec3(blendNearFar);
        if(blendNearFar == 0.0)
            albedo = vec3(0,1,0);
        if(blendNearFar == 1.0)
            albedo = vec3(0,0,1);
    }

    color.rgb = v3FrontColor + albedo * v3FrontSecondaryColor;

    color.a = 1.0f;
}
