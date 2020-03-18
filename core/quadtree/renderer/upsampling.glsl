#version 430
// Kernel
layout(local_size_x = 17, local_size_y = 17, local_size_z = 1) in;

// Child heightmaP
layout(r32f, binding = 0) uniform image2D heightmap;

// Parent heightmap
//layout(binding = 0) uniform sampler2D heightmap_parent;

// noisemap
layout(binding = 1) uniform sampler2D noise;
layout(binding = 2) uniform sampler2D elevationmap;
#define HEIGHT_MAP_X (17)
#define HEIGHT_MAP_Y (17)

uniform vec2 lo;
uniform vec2 hi;

const float freq[13] = {4.03*32,1.96*32,1.01*32, 32.0f/2.03f,
                 32.0f/3.98f, 32.0f/8.01f, 32.0f/15.97f,
                 32.0f/31.98f,32.0f/64.01f,32.0f/128.97f,
                 32.0f/256.07f,32.0f/511.89f,32.0f/1024.22f};

#define EFFECTIVE_HEIGHT (0.005)

float calc_height(vec2 pixel)
{

    // Noise sampler1D
    float density = texture(noise,  pixel ).r;

    for(int i = 0; i < 13; i++)
    {
        density += texture(noise,  pixel * freq[i]*32.0f).r;
        density /= 2.0f;

    }
    density = EFFECTIVE_HEIGHT*clamp((density-0.35),0.0,1.0);

    return density;
}

void main()
{

    // get index in global work group i.e x,y position
    ivec2 p = ivec2(gl_LocalInvocationID.xy);
    if(p.x >= HEIGHT_MAP_X || p.y >= HEIGHT_MAP_Y) return;

    vec2 pixel = (p)/vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1); // map [0,1]

    // map to global texture coordinate
    pixel = lo + ( pixel )*(hi-lo);

    // offset to align to the pixel
    //vec2 offset = 0.5/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    //pixel = offset + pixel*(1.0f - 2.0f*offset);

    // [0, 1]? issue: align texture with pixel

    // Procedure
    float height = calc_height(pixel);

    pixel = (pixel+8.0f)/17.0f + 0.5/vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1);
    height += max(texture(elevationmap,  pixel ).r, 0.0f);

    //debug
    //float height = EFFECTIVE_HEIGHT*texture(noise,  pixel ).r;

    imageStore(heightmap, p, vec4(height,0.0,0.0,0.0));

}
