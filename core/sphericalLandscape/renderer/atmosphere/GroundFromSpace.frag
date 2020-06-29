#version 330 core
//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

out vec4 color;

in vec3 v3FrontColor; // InScatter
in vec3 v3FrontSecondaryColor; // Transmittence
in vec3 skyTransmittence;
in vec3 FragPos;
//in vec3 Normal;
in vec2 TexCoords;

in float blendNearFar;

uniform sampler2D s2Tex1;               // diffusive - 2
uniform sampler2D s2Tex2;               // specular - 3
uniform sampler2D normalmap;               // normal
uniform sampler2D normalmapParent;               // normal
uniform samplerCube s2TexTest;

uniform vec3 v3CameraPos;		// The camera's current position
uniform vec3 v3LightDir;		// The direction vector to the light source

uniform int level;
uniform int hash;
uniform int renderType;

vec2 getRello(int code)
{

    code >>= (2*(level-1));
    return 0.5f*vec2((code>>1)&1, (code)&1);
}

vec4 RotationBetweenVectors(vec3 start, vec3 dest);
void rotateVectorByQuat(inout vec3 v, in vec4 q);

void main ()
{
    vec2 shTexcoord = getRello(hash)+TexCoords/(1.0f + float(level > 0));

    // compute radiance
    vec3 normal = mix(texture(normalmap, TexCoords).xyz, texture(normalmapParent, shTexcoord).xyz, blendNearFar);
    float fCosBeta = clamp(0.1 + dot(vec3(0,1,0), normal), 0.0, 1.0);
    rotateVectorByQuat(normal, RotationBetweenVectors(vec3(0,1,0), FragPos));
    float fCosAlpha = clamp(dot(v3LightDir, normal), 0.0, 1.0);

    // Compute albedo
    vec3 albedo = 0.2*mix(texture( s2Tex1, TexCoords ).rgb,
                texture( s2Tex2, shTexcoord ).rgb,
                blendNearFar);

    if(renderType == 1)
    {
        albedo = vec3(blendNearFar);
        if(blendNearFar == 0.0)
            albedo = vec3(0,1,0);
        if(blendNearFar == 1.0)
            albedo = vec3(0,0,1);
    }
    else if(renderType == 2)
    {
        albedo = normal;
    }
    else if(renderType == 3)
    {
        albedo = 0.2*texture( s2TexTest, FragPos ).rgb;
    }

    // Sum color
    color.rgb = v3FrontColor + albedo * (fCosAlpha * v3FrontSecondaryColor + fCosBeta*skyTransmittence);

    color.a = 1.0f;
}

// rotate vector v by quaternion q; see info [1]
void rotateVectorByQuat(inout vec3 v, in vec4 q)
{
    vec3 t = 2 * cross(q.xyz, v);
    v = v + q.w * t + cross(q.xyz, t);
}

// Inputs have to be normalized vector
vec4 RotationBetweenVectors(vec3 start, vec3 dest){
    //start = normalize(start);
    //dest = normalize(dest);

    float cosTheta = dot(start, dest);
    vec3 rotationAxis;

    if (cosTheta < -1 + 0.001f){
        // special case when vectors in opposite directions:
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        rotationAxis = cross(vec3(0.0f, 0.0f, 1.0f), start);
        if (length(rotationAxis) < 0.0001 ) // bad luck, they were parallel, try again!
            rotationAxis = cross(vec3(1.0f, 0.0f, 0.0f), start);

        rotationAxis = normalize(rotationAxis);
        return vec4(
                    rotationAxis.x,
                    rotationAxis.y,
                    rotationAxis.z,
                    0
                    );
    }

    rotationAxis = cross(start, dest);

    float s = sqrt( (1+cosTheta)*2 );
    float invs = 1 / s;

    return vec4(
                rotationAxis.x * invs,
                rotationAxis.y * invs,
                rotationAxis.z * invs,
                s * 0.5f
                );

}
