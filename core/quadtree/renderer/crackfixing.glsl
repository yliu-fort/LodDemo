#version 430
// Kernel
layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

// Child heightmaP
layout(r32f, binding = 0) uniform image2D heightmap;

// Parent heightmap
layout(binding = 0) uniform sampler2D heightmap_neighbour;

#define HEIGHT_MAP_X (16)
#define HEIGHT_MAP_Y (16)

uniform vec2 mylo;
uniform vec2 myhi;

uniform vec2 shlo;
uniform vec2 shhi;

void main()
{

    // get index in global work group i.e x,y position
    int p = int(gl_LocalInvocationID.x);
    if(p >= HEIGHT_MAP_X) return;

    // map to global texture coordinate
    vec2 m_p = mylo + float(p)*(myhi-mylo); // [0-15]
    vec2 sh_p = shlo + (float(p)/(HEIGHT_MAP_X-1))*(shhi-shlo); // [f1-f2]

    // offset to align to the pixel
    float offset = 0.5/(HEIGHT_MAP_X);
    sh_p = offset + sh_p*(1.0f - 2.0f*offset);

    // sample
    float sh_height = texture(heightmap_neighbour,  sh_p ).r;

    imageStore(heightmap, ivec2(m_p), vec4(sh_height,0.0,0.0,0.0));

}
