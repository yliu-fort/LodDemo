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
in vec3 v3Direction;

uniform vec3 v3LightDir;
uniform float g;
uniform float g2;
//uniform sampler1D phaseTex;

void main ()
{
        float fCos = dot(v3LightDir, v3Direction) / length(v3Direction);
        float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);
        color.rgb = v3FrontColor + fMiePhase * v3FrontSecondaryColor;
        color.a = color.b;

}
