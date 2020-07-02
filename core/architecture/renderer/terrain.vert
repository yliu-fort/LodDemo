#version 430 core
#define HEIGHT_MAP_X (19)
#define HEIGHT_MAP_Y (19)
#define K (2)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out float elevation;
out float blendNearFar;
out vec2 shlo;
out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;


uniform mat4 m4ModelViewProjectionMatrix;
uniform mat4 m4ModelMatrix;
uniform vec3 v3CameraPos;

uniform mat4 m4CubeProjMatrix;
uniform vec3 v3CameraProjectedPos;
uniform int level;
uniform int hash;


layout(binding = 0) uniform sampler2D heightmap;
layout(binding = 1) uniform sampler2D heightmapParent;


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

vec2 computeSharedPixel(ivec2 texel, int code)
{

    // todo: might be better to compute dpos and shlow then pass into the vs
    code >>= (2*(level-1));
    shlo =  0.5f*vec2((code>>1)&1, (code)&1);

    //vec2 rr = (shlo*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1))/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    //vec2 sh_pixel = ( shlo*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)
    //                + (texel*vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y) - 0.5f)/2 + 0.5f )
    //        /vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y); // map to shlo->shhi
    return ( 0.5f + shlo*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) + texel/(1.0f + float(level > 0)) )
            /vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y); // map to shlo->shhi
}


void getNormalAndHeightData(out float h, out vec3 n)
{
    ivec2 texel = ivec2(floor(aTexCoords*vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)));

    // blend (need repair)
    //vec2 gPos = lo + aPos.xz*(hi-lo);
    vec2 gPos = vec2(dpos(hash) + 2*aPos.xz/(1<<level) - v3CameraProjectedPos.xz);
    //float d = max(abs(gPos.x - v3CameraProjectedPos.x),abs(gPos.y - v3CameraProjectedPos.z));
    //float l = 0.5f*dot(hi-lo, vec2(1));
    float d_l = max(abs(gPos.x),abs(gPos.y))*(1<<level)/2;
    blendNearFar = clamp((d_l-K-1)/(K-1),0,1);

    // get values
    vec4 data = mix( texelFetch(heightmap, texel, 0), texture(heightmapParent, computeSharedPixel(texel, hash)), blendNearFar );

    h = data.r;
    n = normalize(vec3(m4ModelMatrix*vec4(data.gba,0.0f)));
}

vec3 projectVertexOntoSphere(float h)
{
    vec3 q = vec3(m4CubeProjMatrix*vec4(aPos,1.0f));
    return vec3( m4ModelMatrix*vec4( (1.0+h)*normalize(q),1.0f ) );
}

void main()
{
    // To avoid artifacts in normal calculation
    TexCoords = aTexCoords;
    getNormalAndHeightData(elevation, Normal);

    // Write to fragpos and height field
    // Project to non-euclidian space (quat-sphereical)
    // popping effect counter measure: compute view based fragpos
    // current we compute a fragpos relative to camera pos
    FragPos = projectVertexOntoSphere(elevation);

    gl_Position = m4ModelViewProjectionMatrix*vec4(FragPos, 1.0);

}
