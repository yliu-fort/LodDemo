#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

#define M_PI (3.141592654)

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 projection_view;
uniform mat4 model;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform sampler2D specularMap;

float height()
{
    return 0.02f*(1.0f - texture(specularMap, aTexCoords).r);
}

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos + aNormal*height(), 1.0));
    vs_out.TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * vec3(sin(2.0f*M_PI*aTexCoords.y)*cos(2.0f*M_PI*aTexCoords.x),
                                           sin(2.0f*M_PI*aTexCoords.y)*sin(2.0f*M_PI*aTexCoords.x),
                                           cos(2.0f*M_PI*aTexCoords.y)));
    vec3 N = normalize(normalMatrix * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));

    //vs_out.FragPos = vec3(model * vec4(aPos + aNormal*z_offset, 1.0));

    vs_out.TangentLightPos = TBN * lightPos;
    vs_out.TangentViewPos  = TBN * viewPos;
    vs_out.TangentFragPos  = TBN * vs_out.FragPos;

    gl_Position = projection_view*vec4(vs_out.FragPos, 1.0);

}
