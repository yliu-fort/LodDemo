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

void main()
{
    // get index in global work group i.e x,y position
    // Sync ghost layer 1
    vec2 PIVOT = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - 2*NUM_GHOST_LAYER + 1 )/2.0; // 14.5
    vec2 RANGE = abs(ivec2(xoffset, yoffset))*(PIVOT + NUM_GHOST_LAYER - 1); // 15.5
    vec2 PIVOTN = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - 2*NUM_GHOST_LAYER - 1 )/2.0; // 13.5
    vec2 RANGEN = abs(ivec2(xoffset, yoffset))*(PIVOTN + NUM_GHOST_LAYER ); // 15.5

    // 1 or FIELD_MAP - 2
    ivec2 dest = ivec2(gl_GlobalInvocationID.xy) + ivec2(vec2( xoffset, yoffset) * PIVOT + RANGE);
    ivec2 src = ivec2(gl_GlobalInvocationID.xy) + ivec2(vec2(-xoffset,-yoffset) * PIVOTN + RANGEN);


    imageStore(f0w, dest, texelFetch(f0, src, 0));
    imageStore(f1w, dest, texelFetch(f1, src, 0));


    //// Sync ghost layer 2
    //// layer 1: (1, 30) -> layer 2: (0, 31)
    //PIVOT = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - NUM_GHOST_LAYER + 1 )/2.0; // 15.5
    //RANGE = abs(ivec2(xoffset, yoffset))*(PIVOT); // 15.5
    //// layer 1: (2, 29) -> layer 2: (3, 28)
    //PIVOTN = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - 2*NUM_GHOST_LAYER - 3 )/2.0; // 12.5
    //RANGEN = abs(ivec2(xoffset, yoffset))*(PIVOTN + NUM_GHOST_LAYER + 1 ); // 15.5
    //
    //// 0 or FIELD_MAP - 1
    //dest = ivec2(gl_GlobalInvocationID.xy) + ivec2(vec2( xoffset, yoffset) * PIVOT + RANGE);
    //src = ivec2(gl_GlobalInvocationID.xy) + ivec2(vec2(-xoffset,-yoffset) * PIVOTN + RANGEN);
    //
    //
    //imageStore(f0w, dest, texelFetch(f0, src, 0));
}
