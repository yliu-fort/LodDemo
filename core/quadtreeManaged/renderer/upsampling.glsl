#version 430
#define HEIGHT_MAP_X (25)
#define HEIGHT_MAP_Y (25)
// Kernel
layout(local_size_x = 25, local_size_y = 25, local_size_z = 1) in;

// Child heightmaP
layout(r32f, binding = 0) uniform image2D heightmap;

// Parent heightmap
//layout(binding = 0) uniform sampler2D heightmap_parent;

// noisemap
layout(binding = 1) uniform sampler2D noise;
layout(binding = 2) uniform sampler2D elevationmap;

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
    float density = texture(noise,  pixel * freq[0] * 4.0f ).r;

    for(int i = 1; i < 12; i++)
    {
        density /= 2.0f;
        density += texture(noise, pixel * freq[i] * 4.0f ).r;
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
    pixel = lo + ( pixel )*(hi-lo); // [lo, hi]

    // offset to align to the pixel
    //vec2 offset = 0.5/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    //pixel = offset + pixel*(1.0f - 2.0f*offset);

    // [0, 1]? issue: align texture with pixel

    // Procedure
    float height = 0.0f;

    // Noise syethesis
    height += calc_height(pixel);

    // Read heightmap
    // Caution: low-res elevation map causes bumpy ground-> truncation issue
    //pixel = (pixel+8.0f)/16.0f;
    //height += max(texture( elevationmap,  pixel ).r, 0.0f);
    //height = clamp(1.7f*height,0.0,0.4);
    height += -tanh(0.02f*abs(dot(pixel,pixel))-0.5);
    height = clamp(1.7f*height,0.0,0.4);

    //debug
    //float height = EFFECTIVE_HEIGHT*texture(noise,  pixel ).r;

    imageStore(heightmap, p, vec4(height,0.0,0.0,0.0));

}
