#version 430 core
#define HEIGHT_MAP_X (19)
#define HEIGHT_MAP_Y (19)
#define K (2)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out float elevation;
out float blend_display;
out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 projection_view;
uniform mat4 model;
uniform mat4 sphereProjection;
uniform vec3 viewPos;
uniform vec3 projPos;

layout(binding = 0) uniform sampler2D heightmap;
layout(binding = 1) uniform sampler2D heightmapParent;

//uniform vec2 lo;
//uniform vec2 hi;

//uniform vec2 shlo;
//uniform vec2 shhi;

uniform int level;
uniform int hash;

vec2 dpos(int code)
{
    vec2 o = vec2(-1);
    for(int i = 0; i < 15; i++)
    {
        o += vec2((code>>1)&1, (code)&1)/float(1<<i);
        code >>= 2;
    }
    return o;
}

vec2 getRello(int code)
{

    code >>= (2*(level-1));
    return 0.5f*vec2((code>>1)&1, (code)&1);
}

vec2 computeSharedPixel(ivec2 texel)
{
    //vec2 rr = (shlo*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1))/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    //vec2 sh_pixel = ( shlo*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)
    //                + (texel*vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y) - 0.5f)/2 + 0.5f )
    //        /vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y); // map to shlo->shhi
    return ( 0.5f + getRello(hash)*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) + texel/(1.0f + float(level > 0)) )
            /vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y); // map to shlo->shhi
}

void main()
{
    // To avoid artifacts in normal calculation
    TexCoords = aTexCoords;
    ivec2 texel = ivec2(floor(TexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)));

    // blend (need repair)
    //vec2 gPos = lo + aPos.xz*(hi-lo);
    vec2 gPos = vec2( dpos(hash) + 2*aPos.xz/float(1<<level) );
    //float d = max(abs(gPos.x - projPos.x),abs(gPos.y - projPos.z));
    //float l = 0.5f*dot(hi-lo, vec2(1));
    float d_l = max(abs(gPos.x - projPos.x),abs(gPos.y - projPos.z))*(1<<level)/2;
    float blend = clamp((d_l-K-1)/(K-1),0,1);
    blend_display = blend;

    // get values
    vec4 data = mix( texture(heightmap, 0.5/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y) + TexCoords*(1-1/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y))), texture(heightmapParent, computeSharedPixel(texel)), blend );
    //vec4 data = mix( texelFetch(heightmap, texel, 0), texture(heightmapParent, computeSharedPixel(texel)), blend );

    elevation = data.r;
    Normal = normalize(vec3(model*vec4(data.gba,0.0f)));

    // Write to fragpos and height field
    // Project to non-euclidian space (quat-sphereical)
    // popping effect counter measure: compute view based fragpos
    //vec3 q = vec3(sphereProjection*vec4(aPos,1.0f));
    //FragPos = vec3( model*vec4( (1.0+elevation)*normalize(q),1.0f ) ) - viewPos;
    FragPos = vec3( model*vec4(gPos.x, elevation, gPos.y, 1.0f) ) - viewPos;


    // debug
    //height = sqrt(dot(TexCoords,TexCoords));
    //fragPos.y = 0.07*sqrt(dot(TexCoords,TexCoords));

    gl_Position = projection_view*vec4(FragPos, 1.0);
    //gl_Position = vec4(FragPos, 1.0);

}
