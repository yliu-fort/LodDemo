#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;

uniform mat4 model;
uniform mat4 projection_view;

#define M_PI 3.1415926535897932384626433832795

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    // Normal mapping
    vec3 T = normalize(vec3(model * vec4(sin(2.0f*M_PI*aTexCoords.y)*cos(2.0f*M_PI*aTexCoords.x),
                                         sin(2.0f*M_PI*aTexCoords.y)*sin(2.0f*M_PI*aTexCoords.x),
                                         cos(2.0f*M_PI*aTexCoords.y), 0.0f)));
    vec3 N = normalize(vec3(model * vec4(Normal, 0.0)));
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(N, T);

    TBN = mat3(T, B, N);

    gl_Position = projection_view * vec4(FragPos, 1.0);
}
