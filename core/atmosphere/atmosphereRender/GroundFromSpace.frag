#version 330 core
//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

out vec4 color;

in vec3 v3FrontColor;
in vec3 v3FrontSecondaryColor;
in vec2 TexCoords;

uniform sampler2D s2Tex1; // diffusive
//uniform sampler2D s2Tex2; // specular


void main ()
{
        color.rgb = v3FrontColor + 0.25 * v3FrontSecondaryColor;
        color.rgb += texture(s2Tex1, TexCoords).rgb * v3FrontSecondaryColor;
}
