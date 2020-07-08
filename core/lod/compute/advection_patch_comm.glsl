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

void main()
{
    // get index in global work group i.e x,y position
    //ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    vec2 PIVOT = ( vec2(FIELD_MAP_X,FIELD_MAP_Y) - 2*NUM_GHOST_LAYER - 1 )/2.0;
    ivec2 p = ivec2(gl_GlobalInvocationID.xy) + ivec2(vec2( xoffset, yoffset) * PIVOT + PIVOT + NUM_GHOST_LAYER);
    ivec2 n = ivec2(gl_GlobalInvocationID.xy) + ivec2(vec2(-xoffset,-yoffset) * PIVOT + PIVOT + NUM_GHOST_LAYER);


    vec3 data = imageLoad(f0w, p).xyz;

    if(xoffset == -1 && yoffset ==  0)
    {
        data.x = texelFetch(f0, n, 0).x; // advect towards +x
        //data.y = texelFetch(f0, n + ivec2(0, -1), 0).y; // advect towards +x+y
    }

    if(xoffset == -1 && yoffset == -1)
        //data.y = texelFetch(f0, n, 0).y; // advect towards +xy

    if(xoffset ==  0 && yoffset == -1)
    {
        //data.y = texelFetch(f0, n + ivec2(-1, 0), 0).y; // advect towards +x+y
        data.z = texelFetch(f0, n, 0).z; // advect towards +y
    }

    imageStore(f0w, p, vec4(data,1.0));
}
