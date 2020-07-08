#version 430

#define NUM_GHOST_LAYER (2)
#define FIELD_MAP_X (28 + 2*NUM_GHOST_LAYER)
#define FIELD_MAP_Y (28 + 2*NUM_GHOST_LAYER)
// Kernel
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// registered fields
layout(rgba32f, binding = 0) uniform image2D f0w;
layout(rgba32f, binding = 1) uniform image2D f1w;

uniform int level;
uniform int hash;

int MergeBits1(int v)
{
    v &= 0x55555555;
    v = (v ^ (v >> 1 )) & 0x33333333;
    v = (v ^ (v >> 2 )) & 0x0F0F0F0F;
    v = (v ^ (v >> 4 )) & 0x00FF00FF;
    v = (v ^ (v >> 8 )) & 0x0000FFFF;
    return v;
}

vec2 DecodeMortonWithLod2(int v, int l)
{
    v <<= (2*l);
    v &= 0x3FFFFFFF;
    v <<= 2;
    return vec2(
                float(MergeBits1(v >> 0 )) / 65536.0f,
                float(MergeBits1(v >> 1 )) / 65536.0f
                );
}

vec2 getCurrentUV()
{
    return 2.0f*DecodeMortonWithLod2(hash, 15-level)-1.0f
            + 2.0*ivec2(gl_GlobalInvocationID.xy - NUM_GHOST_LAYER)
            /vec2(FIELD_MAP_X - 2*NUM_GHOST_LAYER -1, FIELD_MAP_Y - 2*NUM_GHOST_LAYER -1)
            /float(1<<level);
}


void main()
{

    // get index in global work group i.e x,y position
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if(p.x >= FIELD_MAP_X || p.y >= FIELD_MAP_Y) return;

    vec2 pixel = getCurrentUV();

    // Procedure
    float height = clamp( float(pow(length(pixel+1), 2.0f) < 0.5f*0.5f), 0.0f, 1.0f );

    vec3 color = vec3(height);

    // Ghost layer visualization
    //color = vec3(pixel,0);
    //if(p.x < 2 && p.y < 2)
    //color = vec3(1);
    if(p.x == 1 || p.y == 1 || p.x == FIELD_MAP_X-2 || p.y == FIELD_MAP_Y-2)
        color = vec3(0,0,0);
    if(p.x == 0 || p.y == 0 || p.x == FIELD_MAP_X-1 || p.y == FIELD_MAP_Y-1)
        color = vec3(0,0,0);

    imageStore(f0w, p, vec4(color,1.0));
    imageStore(f1w, p, vec4(color,1.0));
}
