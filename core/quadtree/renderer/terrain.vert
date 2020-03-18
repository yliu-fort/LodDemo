#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out float height_display;
out vec2 TexCoords;

uniform mat4 projection_view;
uniform mat4 model;
uniform sampler2D heightmap;

//uniform vec2 lo;
//uniform vec2 hi;

#define HEIGHT_MAP_X (17)
#define HEIGHT_MAP_Y (17)

void main()
{
    //TexCoords = lo + (aTexCoords)*(hi-lo);
    TexCoords = aTexCoords;

    vec3 fragPos = aPos;

    //float height = texture(heightmap, aTexCoords).r;
    float height = texelFetch(heightmap, ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)), 0).r; // pixel accurate but bumpy

    // Write to fragpos and height field
    fragPos.y += height;
    height_display = height;

    // debug
    //height = sqrt(dot(TexCoords,TexCoords));
    //fragPos.y = 0.07*sqrt(dot(TexCoords,TexCoords));


    gl_Position = projection_view*model*vec4(fragPos, 1.0);
}
