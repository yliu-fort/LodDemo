#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 m4ViewProjectionMatrix;
uniform mat4 m4ModelMatrix;

uniform float phi; // +x direction bending
uniform float theta; // +z direction bending
uniform float radius;
void main()
{
    TexCoords = aTexCoords;
    vec3 FragPos = aPos;

    float phi_ = mix(0, phi, TexCoords.x);
    float theta_ = mix(0, 3.14159f/2.0f, TexCoords.y);

    //FragPos = radius*(-vec3(0,1,0) + vec3(sin(phi_)*cos(theta_),cos(phi_),sin(phi_)*sin(theta_)));

    // projection method 1 (shaking)
    //vec3 center = vec3(radius,-radius, 0);
    //FragPos = length(center)*normalize(aPos - center) + center;

    // projection method 2 (shaking)
    //vec3 Apos = vec3(2*TexCoords.x-1,0,2*TexCoords.y-1);
    //vec3 center = vec3(radius, -radius, radius);
    //float l = length(center) / length(Apos - center);
    //FragPos = l*( Apos - (center * (1-1/l)) );

    // projection method 3 (shaking weaker)
    vec3 Apos = vec3(2*TexCoords.x-1,0,2*TexCoords.y-1);
    vec3 Rpos[4];
    Rpos[0] = vec3(-1000,0,-1000);
    Rpos[1] = vec3( 1000,0,-1000);
    Rpos[2] = vec3(-1000,0, 1000);
    Rpos[3] = vec3( 1000,0, 1000);

    vec3 center = vec3(radius, -radius, radius);

    for (int i = 0; i < 4; i++)
    {
        float l = length(center) / length(Rpos[i] - center);
        Rpos[i] = l*( Rpos[i] - (center * (1-1/l)) );
    }

    vec3 Bpos[2];
    Bpos[0] = mix(Rpos[0], Rpos[2], (Apos.z + 1000.0) / 2000.0 );
    Bpos[1] = mix(Rpos[1], Rpos[3], (Apos.z + 1000.0) / 2000.0 );
    FragPos = mix(Bpos[0], Bpos[1], (Apos.x + 1000.0) / 2000.0 );

    //FragPos -= mix(mix(Rpos[0], Rpos[2], 0.5 ), mix(Rpos[1], Rpos[3], 0.5 ), 0.5 );

    gl_Position = m4ViewProjectionMatrix * m4ModelMatrix *vec4(FragPos, 1.0);
}
