#version 430

#define NUM_GHOST_LAYER (2)
#define FIELD_MAP_X (28 + 2*NUM_GHOST_LAYER)
#define FIELD_MAP_Y (28 + 2*NUM_GHOST_LAYER)

// Kernel
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

// registered fields
layout(binding = 0) uniform sampler2D f0;
layout(rgba32f, binding = 0) uniform image2D f0w;

// uniforms
uniform int xoffset;
uniform int yoffset;
uniform int roffset;

void main()
{
    // get index in global work group i.e x,y position
    //ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    vec2 PIVOT = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - 2*NUM_GHOST_LAYER + 1 )/2.0; // 14
    vec2 RANGE = abs(ivec2(xoffset, yoffset))*(PIVOT + NUM_GHOST_LAYER - 1); // 15
    vec2 PIVOTN = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - 2*NUM_GHOST_LAYER - 2 )/2.0; // 13
    vec2 RANGEN = abs(ivec2(xoffset, yoffset))*(PIVOTN + NUM_GHOST_LAYER ); // 15

    // {1 or FIELD_MAP - 1, 0 : FIELD_MAP/2-1 or FIELD_MAP/2 : FIELD_MAP-1}
    ivec2 dest = ivec2(gl_GlobalInvocationID.xy)
            + abs(ivec2(xoffset==0, yoffset==0))*ivec2(roffset&1, (roffset>>1)&1)*ivec2(FIELD_MAP_X,FIELD_MAP_Y)/2
            + ivec2(vec2( -xoffset, -yoffset) * PIVOT + RANGE);

    // use texture bilinear filtering now
    // need to compute a normlized coordinate
    vec2 src = (0.5f+vec2(gl_GlobalInvocationID.xy)
                +ivec2(xoffset==0, yoffset==0)*(2.0f*vec2(roffset&1, (roffset>>1)&1)-1.0f)
                )/vec2(FIELD_MAP_X/2,FIELD_MAP_Y/2)
            + vec2(vec2(xoffset,yoffset) * PIVOTN + RANGEN)/vec2(FIELD_MAP_X,FIELD_MAP_Y);


    imageStore(f0w, dest, texture(f0, src));
}
