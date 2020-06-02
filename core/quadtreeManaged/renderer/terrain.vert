#version 430 core
#define HEIGHT_MAP_X (17)
#define HEIGHT_MAP_Y (17)

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out float height_display;
out float blend_display;
out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 projection_view;
uniform mat4 model;
uniform vec3 viewPos;

layout(binding = 0) uniform sampler2D heightmap;
layout(binding = 1) uniform sampler2D heightmapParent;

uniform vec2 lo;
uniform vec2 hi;

uniform vec2 shlo;
uniform vec2 shhi;

void main()
{
    // To avoid artifacts in normal calculation
    TexCoords = aTexCoords;

    // blend
    vec2 gPos = lo + aPos.xz*(hi-lo);
    float d = max(abs(gPos.x - viewPos.x),abs(gPos.y - viewPos.z));
    float l = 0.5f*dot(hi-lo, vec2(1));
    float blend = clamp((d/l-2-1)/(2-1),0,1);
    blend_display = blend;

    // get values
    //vec2 rr = (shlo*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1))/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    vec2 sh_pixel = ( shlo*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)
                    + ((aTexCoords*vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y) - 0.5f)*(shhi-shlo)+0.5f) )
            /vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y); // map to shlo->shhi
    float height = mix(texture(heightmap, aTexCoords).r,texture(heightmapParent, sh_pixel ).r,blend);
    Normal = mix(texture(heightmap, aTexCoords).gba,texture(heightmapParent, sh_pixel ).gba,blend);
    //float height = texelFetch(heightmap, ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)), 0).r;

    // Write to fragpos and height field
    FragPos = vec3(model*vec4(aPos.x,aPos.y+height,aPos.z,1.0));
    height_display = height;

    // Project to non-euclidian space (cylindrical)
    //vec3 ePos = vec3((1.0f-FragPos.y)*sin(FragPos.x), -(1.0f-FragPos.y)*cos(FragPos.x)+1, FragPos.z);
    //FragPos = ePos;

    // Project to non-euclidian space (quat-sphereical)
    //vec3 ePos = vec3((1.0f+FragPos.y)*sin(FragPos.x/4+3.14159/2)*cos(FragPos.z/4+3.14159/2),
    //                 (1.0f+FragPos.y)*sin(FragPos.x/4+3.14159/2)*sin(FragPos.z/4+3.14159/2)-1,
    //                 (1.0f+FragPos.y)*cos(FragPos.x/4+3.14159/2) );
    //FragPos = ePos;


    // debug
    //height = sqrt(dot(TexCoords,TexCoords));
    //fragPos.y = 0.07*sqrt(dot(TexCoords,TexCoords));

    gl_Position = projection_view*vec4(FragPos, 1.0);
}
