#version 330 core
//
// Atmospheric scattering fragment shader
//
// Author: Sean O'Neil
//
// Copyright (c) 2004 Sean O'Neil
//

#define quat vec4
out vec4 color;

in vec3 v3FrontColor; // InScatter
in vec3 v3FrontSecondaryColor; // Transmittence
in vec3 skyTransmittence;
in vec3 FragPos;
in vec3 sampleCubeDir;
in vec2 TexCoords;
in vec3 sampleTangentDir;

in float blendNearFar;

uniform mat4 m4CubeProjMatrix;
uniform mat4 m4ModelMatrix;
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

vec2 getSharedLower(int code)
{

    code >>= (2*(level-1));
    return 0.5f*vec2((code>>1)&1, (code)&1);
}

quat RotationBetweenVectors(vec3 start, vec3 dest);
quat RotationBetweenVectorsAxis(vec3 start, vec3 dest, vec3 hintAxis);
vec3 rotateVectorByQuat(vec3 v, in quat q);

void main ()
{
    vec2 shTexcoord = getSharedLower(hash)+TexCoords/(1.0f + float(level > 0));

    // compute lighting (bug: normal correction is incorrect)
    vec3 normal = mix(texture(normalmap, TexCoords).xyz, texture(normalmapParent, shTexcoord).xyz, blendNearFar);
    float fCosBeta = clamp(0.1 + dot(vec3(0,1,0), normal), 0.0, 1.0);

    quat rotToFragPos = RotationBetweenVectors(vec3(0,1,0), FragPos);
    vec3 tangent = rotateVectorByQuat(vec3(1,0,0), rotToFragPos);

    normal = rotateVectorByQuat(normal, rotToFragPos);
    normal = rotateVectorByQuat(normal, RotationBetweenVectorsAxis( tangent, sampleTangentDir, FragPos ));

    float fCosAlpha = clamp(dot(v3LightDir, normal), 0.0, 1.0);

    vec3 reflectDir = reflect(-v3LightDir, normal);
    float spec = 0.2f*mix(texture( s2Tex1, TexCoords ).a,
                     texture( s2Tex2, shTexcoord ).a,
                     blendNearFar) * pow(max(dot(normalize(v3CameraPos - FragPos), reflectDir), 0.0), 128.0);

    // Compute albedo
    vec3 albedo = 0.2f*mix(texture( s2Tex1, TexCoords ).rgb,
                           texture( s2Tex2, shTexcoord ).rgb,
                           blendNearFar);

    if(renderType == 1)
    {
        albedo = vec3(fCosAlpha);
    }
    else if(renderType == 2)
    {
        albedo = clamp(normal,0.0,1.0);
    }
    else if(renderType == 3)
    {
        albedo = 0.2*texture( s2TexTest, sampleCubeDir ).rgb;
    }

    // Sum color
    color.rgb = v3FrontColor
            + albedo * (fCosAlpha * v3FrontSecondaryColor + fCosBeta*skyTransmittence)
            + spec * v3FrontSecondaryColor;

    color.a = 1.0f;
}

// rotate vector v by quaternion q; see info [1]
vec3 rotateVectorByQuat(vec3 v, quat q)
{
    vec3 t = 2 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

// Inputs have to be normalized vector
quat RotationBetweenVectors(vec3 start, vec3 dest){
    start = normalize(start);
    dest = normalize(dest);

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
        return quat(
                    rotationAxis.x,
                    rotationAxis.y,
                    rotationAxis.z,
                    0
                    );
    }

    rotationAxis = cross(start, dest);

    float s = sqrt( (1+cosTheta)*2 );
    float invs = 1 / s;

    return quat(
                rotationAxis.x * invs,
                rotationAxis.y * invs,
                rotationAxis.z * invs,
                s * 0.5f
                );

}

// Inputs have to be normalized vector
quat RotationBetweenVectorsAxis(vec3 start, vec3 dest, vec3 hintAxis){
    start = normalize(start);
    dest = normalize(dest);

    float cosTheta = dot(start, dest);
    vec3 rotationAxis;

    if (cosTheta < -1 + 0.001f){
        // special case when vectors in opposite directions:
        // there is no "ideal" rotation axis
        // So guess one; any will do as long as it's perpendicular to start
        rotationAxis = hintAxis;

        rotationAxis = normalize(rotationAxis);
        return quat(
                    rotationAxis.x,
                    rotationAxis.y,
                    rotationAxis.z,
                    0
                    );
    }

    return RotationBetweenVectors(start, dest);

}
