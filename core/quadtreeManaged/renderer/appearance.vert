#version 430
#define ALBEDO_MAP_X (127)
#define ALBEDO_MAP_Y (127)
#define ELEVATION_MAP_RESOLUTION (256)
// Kernel
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Child heightmaP
layout(rgba32f, binding = 0) uniform image2D albedo;

// Parent heightmap
//layout(binding = 0) uniform sampler2D heightmap_parent;

// noisemap
layout(binding = 1) uniform sampler2D noise;
layout(binding = 2) uniform samplerCube elevationmap;
layout(binding = 3) uniform sampler2DArray material;

uniform vec2 lo;
uniform vec2 hi;

uniform mat4 globalMatrix;

uniform int level;

const float freq[13] = {4.03*32,1.96*32,1.01*32, 32.0f/2.03f,
                 32.0f/3.98f, 32.0f/8.01f, 32.0f/15.97f,
                 32.0f/31.98f,32.0f/64.01f,32.0f/128.97f,
                 32.0f/256.07f,32.0f/511.89f,32.0f/1024.22f};

#define EFFECTIVE_HEIGHT_SYNTHETIC (0.005)
#define EFFECTIVE_HEIGHT (0.001)

vec3 convertToDeformed(vec2 t)
{
    return vec3(globalMatrix*vec4(t.x,0.0f,t.y,1.0f));
}

float calc_height(vec2 pixel)
{

    // Noise sampler1D
    float density = texture(noise,  pixel * freq[0] * 4.0f ).r;

    for(int i = 1; i < 12; i++)
    {
        density /= 2.0f;
        density += texture(noise, pixel * freq[i] * 4.0f ).r;
    }

    // Procedure
    density = EFFECTIVE_HEIGHT_SYNTHETIC*clamp((density-1.0),0.0,1.0);

    // Read elevation map
    // Caution: low-res elevation map causes bumpy ground-> truncation issue
    //density += -2.0*tanh(0.03f*abs(dot(pixel,pixel))-0.15);
    //vec2 offset = 0.5/vec2(ELEVATION_MAP_RESOLUTION);
    //vec2 cpixel = offset + pixel*(1.0f - 2.0f*offset);
    vec3 cpixel = convertToDeformed(pixel);
    density += texture( elevationmap,  cpixel ).r;

    // Bound height
    density = clamp(density,0.0,EFFECTIVE_HEIGHT);

    return density;
}

vec3 convertToSphere(vec2 t)
{
    return (1.0f + calc_height(t)) * normalize(convertToDeformed(t));
}

void main()
{

    // get index in global work group i.e x,y position
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if(p.x >= ALBEDO_MAP_X || p.y >= ALBEDO_MAP_Y) return;

    vec2 pixel = (p)/vec2(ALBEDO_MAP_X-1, ALBEDO_MAP_Y-1); // map [0,1]

    // map to global texture coordinate
    pixel = lo + ( pixel )*(hi-lo); // [lo, hi]

    // Procedure
    float height = calc_height(pixel);

    vec3 color = mix(textureLod( material,
                     vec3(pixel*(1<<15), 6.45f*height/EFFECTIVE_HEIGHT), 15-level ).rgb,
                vec3(0.2,0.2,0.7), clamp(tanh(5e-4f/(50*height+5e-5f))-0.1f,0,1));

    imageStore(albedo, p, vec4(color,1.0f));

}
