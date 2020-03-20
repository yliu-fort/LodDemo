#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 projection_view;
uniform mat4 model;
uniform sampler2D heightmap;

#define HEIGHT_MAP_X (17)
#define HEIGHT_MAP_Y (17)

void main()
{
    TexCoords = aTexCoords;

    //float height = texture(heightmap, aTexCoords).r;
    float height = texelFetch(heightmap, ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)), 0).r;

    FragPos = vec3(model*vec4(aPos.x,aPos.y+height,aPos.z,1.0));

    gl_Position = projection_view * vec4(FragPos, 1.0);
}
