#version 330 core
#define HEIGHT_MAP_X (25)
#define HEIGHT_MAP_Y (25)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out float height_display;
out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 projection_view;
uniform mat4 model;
uniform sampler2D heightmap;

//uniform vec2 lo;
//uniform vec2 hi;


void main()
{

    // To avoid artifacts in normal calculation
    TexCoords = 0.5/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y) + aTexCoords * (1.0 - 1.0/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y));

    //FragPos = aPos;

    //float height = texture(heightmap, aTexCoords).r;
    float height = texelFetch(heightmap, ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)), 0).r;

    // Normal
    vec3 e1 = vec3(model*vec4(0.1f/(HEIGHT_MAP_X-1),0.0,0.0,0.0));
    vec3 e2 = vec3(model*vec4(0.0,0.0,0.1f/(HEIGHT_MAP_Y-1),0.0));

    e1.y = texture( heightmap, TexCoords + vec2(0.05f,0.0f)/vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) ).r
         - texture( heightmap, TexCoords - vec2(0.05f,0.0f)/vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) ).r;
    e2.y = texture( heightmap, TexCoords + vec2(0.0f,0.05f)/vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) ).r
         - texture( heightmap, TexCoords - vec2(0.0f,0.05f)/vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) ).r;
    Normal = normalize(-cross(e1,e2));

    // Write to fragpos and height field
    FragPos = vec3(model*vec4(aPos.x,aPos.y+height,aPos.z,1.0));
    height_display = height;

    // debug
    //height = sqrt(dot(TexCoords,TexCoords));
    //fragPos.y = 0.07*sqrt(dot(TexCoords,TexCoords));


    gl_Position = projection_view*vec4(FragPos, 1.0);
}
