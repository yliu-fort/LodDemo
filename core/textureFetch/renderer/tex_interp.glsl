#version 430
// Kernel
layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

// small map
layout(r32f, binding = 0) uniform image2D smallMap;

// large map
layout(binding = 0) uniform sampler2D largeMap;


#define HEIGHT_MAP_X (16)
#define HEIGHT_MAP_Y (16)

uniform vec2 lo;
uniform vec2 hi;


void main()
{

    // fine map ccd -> non-normalize
    vec2 lo = vec2(1.0,0.0) * vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1);
    vec2 hi = vec2(1.0,1.0) * vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1);

    // coarse map ccd
    vec2 shlo = vec2(0.0,0.5);
    vec2 shhi = vec2(0.0,1.0);

    // get index in global work group i.e x,y position
    int p = int(gl_LocalInvocationID.x);
    if(p>= HEIGHT_MAP_Y) return;

    // map to global texture coordinate
    vec2 pixel = lo + ((p)/float(HEIGHT_MAP_Y-1))*(hi-lo); // map to lo->hi
    vec2 sh_pixel = shlo + ((p*(shhi-shlo) + 0.5)/float(HEIGHT_MAP_Y)); // map to shlo->shhi

    // offset to align to the pixel
    //vec2 offset = 0.5/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    //sh_pixel = offset + sh_pixel;//*(1.0f - 2.0f*offset);

    //debug
    float height = texture(largeMap,  sh_pixel ).r;

    imageStore(smallMap, ivec2(pixel), vec4(height,0.0,0.0,0.0));

}
