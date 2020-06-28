#version 330 core
//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//
in vec2 TexCoords;

out vec4 color;

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
//uniform mat4 m4ModelViewProjectionMatrix;
uniform mat4 m4ModelMatrix;
uniform float fESun;			// ESun

const int nSamples = 16;
const float fSamples = 16.0;
const float PI = 3.141592654;

uniform sampler2D opticalTex;

vec2 getRayleigh(float fCos, float fHeight)
{
    float x = (1.0f-fCos)/2.0f;
    float y = (fHeight - fInnerRadius)*fScale;

    return texture(opticalTex, vec2(y,x)).xy;
}


uniform sampler2D s2Tex1;               // diffusive
//uniform sampler2D s2Tex2;               // specular

const vec3 groundAlbedo = vec3(0,0,0.04);

void main ()
{
    // Get the ray from the camera to the vertex, and its length (which is the far point of the ray passing through the atmosphere)
    // sweep camera angle from -1 to 1
    float s = TexCoords.x*PI/2;
    float t = (1 - TexCoords.y)*PI/2;
    vec3 v3Ray = vec3(sin(t)*cos(s), cos(t), sin(t)*sin(s));
    //v3Ray = normalize(vec3(m4ModelMatrix*vec4(v3Ray,0.0f)));
    //vec3 v3Ray = vec3(sin(t), cos(t), 0.0f);

    // find intersection to innerSphere / outerSphere
    // compute intersection to the outersphere when out of atmosphere
    vec3 v3Start = v3CameraPos;

    // Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)
    float B = 2.0 * dot(v3CameraPos, v3Ray);
    float C = fCameraHeight2 - fInnerRadius2;
    float fDet = max(0.0, B*B - 4.0 * C);
    float fNear = 0.5 * (-B - sqrt(fDet));
    bool hitGround = (B*B - 4.0 * C) >= 0 && fNear >= 0;
    if(!hitGround)
    {
        C = fCameraHeight2 - fOuterRadius2;
        fDet = max(0.0, B*B - 4.0 * C);
        fNear = 0.5 * (-B + sqrt(fDet));
    }


    // Calculate the ray's starting position, then calculate its scattering offset
    vec3 v3Pos = v3CameraPos + v3Ray * fNear;
    float fFar = fNear;

    float fCameraOffset = 0;
    if(hitGround)
        fCameraOffset = getRayleigh(dot(-v3Ray, v3CameraPos) / fCameraHeight,fCameraHeight).y;
    else
        fCameraOffset = getRayleigh(dot( v3Ray, v3CameraPos) / fCameraHeight,fCameraHeight).y;


    // Now loop through the sample rays
    float fSampleLength = fFar / nSamples;
    vec3 v3SampleRay = v3Ray * fSampleLength;
    float fScaledLength = fSampleLength * fScale;
    vec3 v3SamplePoint = v3Start + 0.5*v3SampleRay;

    // Now loop through the sample rays
    vec3 inScatter = vec3(0.0, 0.0, 0.0);
    vec3 transmittence = vec3(0.0, 0.0, 0.0);
    for(int i=0; i<nSamples; i++)
    {
        float fHeight = length(v3SamplePoint);
        float fLightAngle = dot(v3LightDir, v3SamplePoint) / fHeight;

        float Rho = getRayleigh(fLightAngle, fHeight).x;

        if(hitGround)
        {
            float fCameraAngle = dot(-v3Ray, v3SamplePoint) / fHeight;
            float fScatter = getRayleigh(fLightAngle, fHeight).y + getRayleigh(fCameraAngle, fHeight).y - fCameraOffset;
            transmittence = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
        }
        else
        {
            float fCameraAngle = dot(v3Ray, v3SamplePoint) / fHeight;
            float fScatter = getRayleigh(fLightAngle, fHeight).y - getRayleigh(fCameraAngle, fHeight).y + fCameraOffset;
            transmittence = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
        }


        inScatter += transmittence * (Rho * fScaledLength);
        v3SamplePoint += v3SampleRay;

    }

    inScatter *= (v3InvWavelength * fKrESun + fKmESun);
    transmittence *= fESun * vec3(hitGround) * max(0.0, dot(v3Pos, v3LightDir));

    // Final blending
    color.rgb = inScatter + groundAlbedo * transmittence;
    color.a = 1.0f;

}
