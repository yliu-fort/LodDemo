#version 330 core
//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//
#define quat vec4
out vec4 color;

in vec3 v3FrontColor; // InScatter
in vec3 v3FrontSecondaryColor; // Transmittence
in vec3 skyTransmittence;
in vec3 FragPos;
in vec3 sampleCubeDir;
in vec2 TexCoords;

in float blendNearFar;

uniform samplerCube s2TexTest;

uniform vec3 v3CameraPos;		// The camera's current position
uniform vec3 v3LightDir;		// The direction vector to the light source

uniform int renderType;

void main ()
{
    // compute lighting (bug: normal correction is incorrect)
    vec3 normal = FragPos;
    float fCosBeta = 1.0f;
    float fCosAlpha = clamp(dot(v3LightDir, normal), 0.0f, 1.0f);

    vec3 reflectDir = reflect(-v3LightDir, normal);
    float spec = 0.2f * pow(max(dot(normalize(v3CameraPos - FragPos), reflectDir), 0.0f), 128.0f);

    // Compute albedo
    vec3 albedo = 0.2f*vec3(0.0,0.0,0.04);

    if(renderType == 1)
    {
        albedo = vec3(fCosAlpha);
    }
    else if(renderType == 2)
    {
        albedo = clamp(normal,0.0,1.0);
    }
    else if(renderType == 3)
    {
        albedo = 0.2*texture( s2TexTest, sampleCubeDir ).rgb;
    }

    // Sum color
    color.rgb = v3FrontColor
            + albedo * (fCosAlpha * v3FrontSecondaryColor + fCosBeta*skyTransmittence)
            + spec * v3FrontSecondaryColor;


    color.a = 1.0f;
}


