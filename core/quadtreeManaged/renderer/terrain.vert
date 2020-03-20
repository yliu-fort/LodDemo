#version 430 core
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
layout(binding = 0) uniform sampler2D heightmap;

//uniform vec2 lo;
//uniform vec2 hi;


void main()
{
    vec2 s = 1.0f/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    // To avoid artifacts in normal calculation
    TexCoords = 0.5*s + aTexCoords * (1.0 - s);

    //float height = texture(heightmap, aTexCoords).r;
    float height = texelFetch(heightmap, ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)), 0).r;

    // on-the-fly normal
    vec2 t1 = clamp(TexCoords + vec2(0.01f,0.0f)*s,vec2(0.5)*s,vec2(1.0f-0.5*s) );
    vec2 t2 = clamp(TexCoords - vec2(0.01f,0.0f)*s,vec2(0.5)*s,vec2(1.0f-0.5*s) );
    vec2 t3 = clamp(TexCoords + vec2(0.0f,0.01f)*s,vec2(0.5)*s,vec2(1.0f-0.5*s) );
    vec2 t4 = clamp(TexCoords - vec2(0.0f,0.01f)*s,vec2(0.5)*s,vec2(1.0f-0.5*s) );

    vec3 e1 = vec3(model*vec4((t1.x-t2.x),0.0,0.0,0.0));
    vec3 e2 = vec3(model*vec4(0.0,0.0,(t3.y-t4.y),0.0));

    e1.y = texture( heightmap, t1 ).r - texture( heightmap, t2 ).r;
    e2.y = texture( heightmap, t3 ).r - texture( heightmap, t4 ).r;
    Normal = normalize(-cross(e1,e2));

    //ivec2 t1 = clamp(ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)) + ivec2( 1,0),ivec2(0),ivec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) );
    //ivec2 t2 = clamp(ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)) - ivec2( 1,0),ivec2(0),ivec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) );
    //ivec2 t3 = clamp(ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)) + ivec2( 0,1),ivec2(0),ivec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) );
    //ivec2 t4 = clamp(ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)) - ivec2( 0,1),ivec2(0),ivec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) );

    //vec3 e1 = vec3(model*vec4(float(t1.x-t2.x)/HEIGHT_MAP_X,0.0,0.0,0.0));
    //vec3 e2 = vec3(model*vec4(0.0,0.0,float(t3.y-t4.y)/HEIGHT_MAP_Y,0.0));

    // Write to fragpos and height field
    FragPos = vec3(model*vec4(aPos.x,aPos.y+height,aPos.z,1.0));
    height_display = height;

    // debug
    //height = sqrt(dot(TexCoords,TexCoords));
    //fragPos.y = 0.07*sqrt(dot(TexCoords,TexCoords));

    gl_Position = projection_view*vec4(FragPos, 1.0);
}
