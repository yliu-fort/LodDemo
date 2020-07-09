#version 430

#define NUM_GHOST_LAYER (2)
#define FIELD_MAP_X (28 + 2*NUM_GHOST_LAYER)
#define FIELD_MAP_Y (28 + 2*NUM_GHOST_LAYER)

// Kernel
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// registered fields
layout(binding = 0) uniform sampler2D f0;
layout(rgba32f, binding = 0) uniform image2D f0w;
layout(binding = 1) uniform sampler2D f1;
layout(rgba32f, binding = 1) uniform image2D f1w;

// uniforms
uniform int xoffset;
uniform int yoffset;
uniform int roffset;

void main()
{
    // get index in global work group i.e x,y position
    //ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    vec2 PIVOT = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - NUM_GHOST_LAYER )/2.0f; // 15
    vec2 RANGE = abs(vec2(xoffset, yoffset))*(PIVOT); // 15
    vec2 PIVOTN = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - 2*NUM_GHOST_LAYER - 1 )/2.0f; // 13.5
    vec2 RANGEN = abs(vec2(xoffset, yoffset))*(PIVOTN + NUM_GHOST_LAYER ); // 15.5


    // {0 ,0} ~ {0, 15}
    ivec2 dest = ivec2(gl_GlobalInvocationID.xy)
            + ivec2(vec2( -xoffset, -yoffset) * PIVOT + RANGE);
    dest = dest * (1+ivec2(xoffset==0, yoffset==0)); // expand to 0 - 30

    // notice that 1 - offset is applied to coarse index
    // 1 ~ 16 / 16 - 31
    ivec2 src = ivec2(gl_GlobalInvocationID.xy)
            + ivec2(xoffset==0, yoffset==0)*(1 + ivec2(roffset&1, (roffset>>1)&1)*(ivec2(FIELD_MAP_X/2,FIELD_MAP_Y/2)-2))
            + ivec2(vec2(xoffset, yoffset) * PIVOTN + RANGEN);


    // Extrapolation
    vec4 data = texelFetch(f0, src, 0); // +1 ?
    imageStore(f0w, dest+ivec2(0,0), data);
    imageStore(f0w, dest+ivec2(1,0), data);
    imageStore(f0w, dest+ivec2(0,1), data);
    imageStore(f0w, dest+ivec2(1,1), data);

    data = texelFetch(f1, src, 0);
    imageStore(f1w, dest           , data);
    imageStore(f1w, dest+ivec2(1,0), data);
    imageStore(f1w, dest+ivec2(0,1), data);
    imageStore(f1w, dest+ivec2(1,1), data);
}
