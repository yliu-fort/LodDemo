#version 330 core
//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

out vec4 color;

in vec3 v3FrontColor; // Scattered by atmosphere
in vec3 v3FrontSecondaryColor; // Attenuation
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D s2Tex1;               // diffusive
//uniform sampler2D s2Tex2;               // specular

uniform vec3 v3CameraPos;		// The camera's current position
uniform vec3 v3LightDir;		// The direction vector to the light source

void main ()
{

    // diffuse
    //float diffuse = max(dot(v3LightDir, Normal), 0.0);

    //color.rgb = v3FrontColor + 0.25 * v3FrontSecondaryColor;
    color.rgb = v3FrontColor + texture(s2Tex1, TexCoords).rgb * v3FrontSecondaryColor;

    //color.rgb *= diffuse;

    color.a = 1.0f;
}
