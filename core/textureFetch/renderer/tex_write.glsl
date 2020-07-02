#version 430
// Kernel
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// small map
layout(binding = 0, r32f) writeonly uniform image2D smallMap;

// large map
layout(location = 0) uniform sampler2D largeMap;

#define HEIGHT_MAP_X (16)
#define HEIGHT_MAP_Y (16)



void main()
{

    // fine map ccd -> non-normalize
    vec2 lo = vec2(0.0,0.0);
    vec2 hi = vec2(1.0,1.0);

    // get index in global work group i.e x,y position
    vec2 p = (0.5f + vec2(gl_LocalInvocationID.xy))/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);


    // map to global texture coordinate
    vec2 pixel = lo + p * (hi-lo); // map to lo->hi

    //debug
    float height = texture( largeMap,  pixel ).r;

    imageStore(smallMap, ivec2(gl_LocalInvocationID.xy), vec4(height,0.0,0.0,0.0));

}
