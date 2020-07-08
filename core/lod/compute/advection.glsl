#version 430

#define NUM_GHOST_LAYER (2)
#define FIELD_MAP_X (28 + 2*NUM_GHOST_LAYER)
#define FIELD_MAP_Y (28 + 2*NUM_GHOST_LAYER)

// Kernel
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// registered fields
layout(binding = 0) uniform sampler2D f0;
layout(rgba32f, binding = 0) uniform image2D f0w;
layout(binding = 1) uniform sampler2D f1;
layout(rgba32f, binding = 1) uniform image2D f1w;

void main()
{
    // get index in global work group i.e x,y position
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if(p.x > FIELD_MAP_X-NUM_GHOST_LAYER || p.y > FIELD_MAP_Y-NUM_GHOST_LAYER ) return;
    if(p.x < NUM_GHOST_LAYER-1 || p.y < NUM_GHOST_LAYER-1 ) return;

    vec3 data;
    data.x = texelFetch(f0, p + ivec2(-1,  0), 0).x; // advect towards +x
    data.y = texelFetch(f0, p + ivec2(-1, -1), 0).y; // advect towards +xy
    data.z = texelFetch(f0, p + ivec2( 0, -1), 0).z; // advect towards +y

    imageStore(f0w, p, vec4(data,1.0));


    data.x = texelFetch(f1, p + ivec2( 1,  0), 0).x; // advect towards +x
    data.y = texelFetch(f1, p + ivec2( 1,  1), 0).y; // advect towards +xy
    data.z = texelFetch(f1, p + ivec2( 0,  1), 0).z; // advect towards +y

    imageStore(f1w, p, vec4(data,1.0));
}