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

out vec3 v3FrontColor;
out vec3 v3FrontSecondaryColor;
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform vec3 v3CameraPos;		// The camera's current position
uniform vec3 v3LightDir;		// The direction vector to the light source
uniform vec3 v3InvWavelength;	// 1 / pow(wavelength, 4) for the red, green, and blue channels
uniform float fCameraHeight;	// The camera's current height
uniform float fCameraHeight2;	// fCameraHeight^2
uniform float fOuterRadius;		// The outer (atmosphere) radius
uniform float fOuterRadius2;	// fOuterRadius^2
uniform float fInnerRadius;		// The inner (planetary) radius
uniform float fInnerRadius2;	// fInnerRadius^2
uniform float fKrESun;			// Kr * ESun
uniform float fKmESun;			// Km * ESun
uniform float fKr4PI;			// Kr * 4 * PI
uniform float fKm4PI;			// Km * 4 * PI
uniform float fScale;			// 1 / (fOuterRadius - fInnerRadius)
uniform float fScaleDepth;		// The scale depth (i.e. the altitude at which the atmosphere's average density is found)
uniform float fScaleOverScaleDepth;	// fScale / fScaleDepth
uniform mat4 m4ModelViewProjectionMatrix;
uniform mat4 m4ModelMatrix;
uniform float fESun;			// ESun

const int nSamples = 4;
const float fSamples = 4.0;
const float PI = 3.141592654;

uniform sampler2D opticalTex;
uniform sampler2D s2Tex3;

float scale(float fCos)
{
    float x = 1.0 - fCos;
    return fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

vec2 getRayleigh(float fCos, float fHeight)
{
    float x = (1.0f-fCos)/2.0f;
    float y = (fHeight - fInnerRadius)*fScale;

    return texture(opticalTex, vec2(y,x)).xy;
}

void main()
{
    // Get the ray from the camera to the vertex, and its length (which is the far point of the ray passing through the atmosphere)
    vec3 aaPos = aPos + 0.003*aNormal*(texture(s2Tex3, aTexCoords).xyz - 0.5);
    vec3 v3Pos = vec3(m4ModelMatrix*vec4(aaPos,1.0)); // Fragpos
    vec3 v3Ray = v3Pos - v3CameraPos;
    //if(length(v3Pos) > length(v3CameraPos))
    //    v3Ray = -v3Ray;
    float fFar = length(v3Ray);
    v3Ray /= fFar;

    // Calculate the ray's starting position, then calculate its scattering offset
    vec3 v3Start = v3CameraPos;
    //float fDepth = exp((fInnerRadius - fCameraHeight) / fScaleDepth);
    //float fCameraAngle = dot(-v3Ray, v3Pos) / length(v3Pos);
    float fCameraAngle = dot(-v3Ray, v3CameraPos) / fCameraHeight;
    float fStartOffset = getRayleigh(fCameraAngle, fCameraHeight).y;

    // Initialize the scattering loop variables
    float fSampleLength = fFar / fSamples;
    float fScaledLength = fSampleLength * fScale;
    vec3 v3SampleRay = v3Ray * fSampleLength;
    vec3 v3SamplePoint = v3Start + v3SampleRay * 0.5;

    // Now loop through the sample rays
    v3FrontColor = vec3(0.0, 0.0, 0.0);
    vec3 v3Attenuate;
    for(int i=0; i<nSamples; i++)
    {
        float fHeight = length(v3SamplePoint);
        float fLightAngle = dot(v3LightDir, v3SamplePoint) / fHeight;
        float fCameraAngle = dot(-v3Ray, v3SamplePoint) / fHeight;

        //float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
        //float fScatter = fDepth*fTemp - fCameraOffset;
        float fDepth = getRayleigh(fLightAngle, fHeight).x;

        float fScatter = getRayleigh(fLightAngle, fHeight).y + getRayleigh(fCameraAngle, fHeight).y - fStartOffset;

        v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
        v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
        v3SamplePoint += v3SampleRay;
    }

    v3FrontColor *= (v3InvWavelength * fKrESun + fKmESun);
    v3FrontSecondaryColor = v3Attenuate * fESun / PI;

    gl_Position = m4ModelViewProjectionMatrix * vec4(aaPos,1.0);
    FragPos = v3Pos;
    Normal = normalize(vec3(m4ModelMatrix*vec4(aNormal,0.0)));
    Normal = aNormal;
    TexCoords = aTexCoords;

}
