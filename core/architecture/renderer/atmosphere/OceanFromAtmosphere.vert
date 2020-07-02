#version 330 core
//
// Atmospheric scattering vertex shader
//
// Author: Sean O'Neil
// Author: Yuxuan Liu
#define HEIGHT_MAP_X (19)
#define HEIGHT_MAP_Y (19)
#define K (2)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 v3FrontColor;
out vec3 v3FrontSecondaryColor;
out vec3 skyTransmittence;
out vec3 FragPos;
out vec3 sampleCubeDir;
out vec2 TexCoords;

out float blendNearFar;

uniform vec3 v3CameraPos;		// The camera's current position
uniform vec3 v3LightDir;		// The direction vector to the light source
uniform vec3 v3InvWavelength;	// 1 / pow(wavelength, 4) for the red, green, and blue channels
uniform float fCameraHeight;	// The camera's current height
uniform float fCameraHeight2;	// fCameraHeight^2
uniform float fOuterRadius;		// The outer (atmosphere) radius
uniform float fOuterRadius2;	// fOuterRadius^2
uniform float fInnerRadius;		// The inner (planetary) radius
uniform float fInnerRadius2;	// fInnerRadius^2
uniform float fESun;			// ESun
uniform float fKrESun;			// Kr * ESun
uniform float fKmESun;			// Km * ESun
uniform float fKr4PI;			// Kr * 4 * PI
uniform float fKm4PI;			// Km * 4 * PI
uniform float fScale;			// 1 / (fOuterRadius - fInnerRadius)
uniform float fScaleDepth;		// The scale depth (i.e. the altitude at which the atmosphere's average density is found)
uniform float fScaleOverScaleDepth;	// fScale / fScaleDepth
uniform mat4 m4ModelViewProjectionMatrix;
uniform mat4 m4ModelMatrix;

const int nSamples = 4;
const float fSamples = 4.0;
const float PI = 3.141592654;

uniform sampler2D opticalTex; // 4

uniform mat4 m4CubeProjMatrix;
uniform vec3 v3CameraProjectedPos;


vec3 projectToS3()
{
    return normalize(vec3(m4CubeProjMatrix*vec4(aPos,1.0f)));
}

vec3 projectVertexOntoSphere(float h)
{
    return vec3( m4ModelMatrix*vec4( (1.0+h)*projectToS3(),1.0f ) );
}

vec3 projectToS3v(vec3 v)
{
    return normalize(vec3(m4CubeProjMatrix*vec4(v,0.0f)));
}

vec3 projectVertexOntoSpherev(vec3 v)
{
    return normalize(vec3( m4ModelMatrix*vec4( projectToS3v(v),0.0f ) ));
}

float scale(float fCos)
{
    float x = 1.0 - fCos;
    return fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

vec2 getRayleigh(float fCos, float fHeight)
{
    float x = (1.0f-fCos)/2.0f;
    float y = (fHeight - fInnerRadius) * fScale;

    return texture(opticalTex, vec2(y,x)).xy;
}

void logarithmicDepthMapping(float far)
{
    // logarithmic depth mapping
    float c = 1;
    gl_Position.z = 2.0*log(gl_Position.w*c + 1)/log(far*c + 1) - 1;
    gl_Position.z *= gl_Position.w;
}

void main()
{
    // Retrieve elevation and normal from texture
    float elevation = 0;

    // Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
    vec3 v3Pos = projectVertexOntoSphere(elevation);
    vec3 v3Ray = v3Pos - v3CameraPos;
    float fFar = length(v3Ray);
    v3Ray /= fFar;

    // Calculate the ray's starting position, then calculate its scattering offset
    vec3 v3Start = v3CameraPos;

    float fHeight = length(v3Pos);
    float fCameraAngle = dot(-v3Ray, v3CameraPos) / fCameraHeight;
    float fStartOffset = getRayleigh(fCameraAngle, fCameraHeight).y;

    // Flip direction of ray when point is above the camera
    if(fHeight > fCameraHeight)
    {
        v3Ray = -v3Ray;
        v3Start = v3Pos;
        fCameraAngle = dot(-v3Ray, v3Pos) / fHeight;
        fStartOffset = getRayleigh(fCameraAngle, fHeight).y;
    }

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

        float fDepth = getRayleigh(fLightAngle, fHeight).x;

        float fScatter = getRayleigh(fLightAngle, fHeight).y + getRayleigh(fCameraAngle, fHeight).y - fStartOffset;

        v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
        v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
        v3SamplePoint += v3SampleRay;
    }

    v3FrontColor = v3FrontColor * (v3InvWavelength * fKrESun + fKmESun);

    // Compute transmittence
    //float fCosAlpha = clamp(dot(v3LightDir, Normal), 0.0, 1.0);
    //float fCosBeta = clamp(0.1 + dot(v3Pos, Normal) / fHeight, 0.0, 1.0);
    skyTransmittence = vec3(0.0,0.05,0.2)*exp(-(getRayleigh(fCameraAngle, fHeight).y - fStartOffset) * (v3InvWavelength * fKr4PI + fKm4PI));

    v3FrontSecondaryColor = v3Attenuate * fESun / PI;

    gl_Position = m4ModelViewProjectionMatrix * vec4(v3Pos,1.0);
    TexCoords = aTexCoords;
    FragPos = v3Pos;
    sampleCubeDir = projectToS3();

    // logarithmic depth mapping
    // https://outerra.blogspot.com/2012/11/maximizing-depth-buffer-range-and.html
    logarithmicDepthMapping(1000);
}
