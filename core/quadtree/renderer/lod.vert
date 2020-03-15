#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 projection_view;
uniform mat4 model;
uniform sampler2D heightmap;

void main()
{
    TexCoords = aTexCoords;
    FragPos = vec3(model * vec4(aPos, 1.0f));

    //float height = texelFetch(heightmap, ivec2(aTexCoords*15),0).r;
    float height = texture(heightmap, aTexCoords).r;

    gl_Position = projection_view * model *vec4(aPos+vec3(0,height,0), 1.0);
}
