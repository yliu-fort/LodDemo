#version 330 core
#define HEIGHT_MAP_X (25)
#define HEIGHT_MAP_Y (25)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;

out VS_OUT {
    vec3 normal;
} vs_out;

uniform mat4 projection_view;
uniform mat4 model;
uniform sampler2D heightmap;

void main()
{
    // on-the-fly normal
    vec2 s = 1.0f/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    vec2 t1 = clamp(0.5*s + aTexCoords * (1.0 - s) + vec2(0.1f,0.0f)*s,vec2(0.5)*s,vec2(1.0f-0.5*s) );
    vec2 t2 = clamp(0.5*s + aTexCoords * (1.0 - s) - vec2(0.1f,0.0f)*s,vec2(0.5)*s,vec2(1.0f-0.5*s) );
    vec2 t3 = clamp(0.5*s + aTexCoords * (1.0 - s) + vec2(0.0f,0.1f)*s,vec2(0.5)*s,vec2(1.0f-0.5*s) );
    vec2 t4 = clamp(0.5*s + aTexCoords * (1.0 - s) - vec2(0.0f,0.1f)*s,vec2(0.5)*s,vec2(1.0f-0.5*s) );

    vec3 e1 = vec3(model*vec4((t1.x-t2.x),0.0,0.0,0.0));
    vec3 e2 = vec3(model*vec4(0.0,0.0,(t3.y-t4.y),0.0));

    e1.y = texture( heightmap, t1 ).r - texture( heightmap, t2 ).r;
    e2.y = texture( heightmap, t3 ).r - texture( heightmap, t4 ).r;
    vec3 Normal = normalize(-cross(e1,e2));
    vs_out.normal = vec3(projection_view * vec4(Normal,0.0));

    // Texture coordinate
    TexCoords = aTexCoords;

    // Heightmap
    float height = texelFetch(heightmap, ivec2(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)), 0).r;

    // Modify fragpos
    FragPos = vec3(model*vec4(aPos.x,aPos.y+height,aPos.z,1.0));

    gl_Position = projection_view * vec4(FragPos, 1.0);
}
