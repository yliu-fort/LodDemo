#version 330 core
//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

out vec4 color;

in vec3 v3FrontSecondaryColor;
in vec2 TexCoords;

uniform sampler2D s2Test;


void main ()
{
        //color = vec4(v3FrontSecondaryColor * texture(s2Test, TexCoords).rgb, 1.0);
        color = vec4(v3FrontSecondaryColor,1.0);
}
