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
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D s2Tex1;               // diffusive
uniform sampler2D s2Tex2;               // specular

uniform vec3 v3CameraPos;		// The camera's current position
uniform vec3 v3LightDir;		// The direction vector to the light source

void main ()
{
    // ambient
    const float ambient = 0.001;

    // specular
    vec3 viewDir = normalize(v3CameraPos - FragPos);
    vec3 reflectDir = reflect(-v3LightDir, Normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.1 * spec * texture(s2Tex2, TexCoords).rgb;


    //color.rgb = v3FrontColor + 0.25 * v3FrontSecondaryColor;
    color.rgb = v3FrontColor + texture(s2Tex1, TexCoords).rgb * (ambient + v3FrontSecondaryColor) + specular;
    color.a = 1.0f;
}
