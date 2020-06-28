#version 330 core
//
// Atmospheric scattering vertex shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;

uniform mat4 m4ModelViewProjectionMatrix;
uniform mat4 m4ModelMatrix;
uniform mat4 m4CubeProjMatrix;

vec3 projectVertexOntoSphere(float h)
{
    vec3 q = vec3(m4CubeProjMatrix*vec4(aPos,1.0f));
    return vec3( m4ModelMatrix*vec4( (1.0+h)*normalize(q),1.0f ) );
}

void main()
{
    // Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
    FragPos = projectVertexOntoSphere(0);
    gl_Position = m4ModelViewProjectionMatrix * vec4(FragPos,1.0);
}
