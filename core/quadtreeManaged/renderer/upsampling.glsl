#version 430
#define HEIGHT_MAP_X (17)
#define HEIGHT_MAP_Y (17)
#define ELEVATION_MAP_RESOLUTION (256)
// Kernel
layout(local_size_x = 17, local_size_y = 17, local_size_z = 1) in;

// Child heightmaP
layout(rgba32f, binding = 0) uniform image2D heightmap;

// Parent heightmap
//layout(binding = 0) uniform sampler2D heightmap_parent;

// noisemap
layout(binding = 1) uniform sampler2D noise;
layout(binding = 2) uniform sampler2D elevationmap;

uniform vec2 lo;
uniform vec2 hi;

uniform mat4 globalMatrix;

const float freq[13] = {4.03*32,1.96*32,1.01*32, 32.0f/2.03f,
                 32.0f/3.98f, 32.0f/8.01f, 32.0f/15.97f,
                 32.0f/31.98f,32.0f/64.01f,32.0f/128.97f,
                 32.0f/256.07f,32.0f/511.89f,32.0f/1024.22f};

#define EFFECTIVE_HEIGHT_SYNTHETIC (0.005)
#define EFFECTIVE_HEIGHT (0.05)

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
    vec2 offset = 0.5/vec2(ELEVATION_MAP_RESOLUTION);
    vec2 cpixel = offset + pixel*(1.0f - 2.0f*offset);
    density += texture( elevationmap,  cpixel ).r;

    // Bound height
    density = clamp(density,0.0,EFFECTIVE_HEIGHT);

    return density;
}

vec3 convertToSphere(vec2 t)
{
    return (1.0f + calc_height(t)) * normalize(vec3(globalMatrix*vec4(t.x,0.0f,t.y,1.0f)));
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
    float height = calc_height(pixel);

    // Noise syethesis
    //height += calc_height(pixel);

    // Read heightmap
    // Caution: low-res elevation map causes bumpy ground-> truncation issue
    //pixel = (pixel+8.0f)/16.0f;
    //height += max(texture( elevationmap,  pixel ).r, 0.0f);
    //height = clamp(1.7f*height,0.0,0.4);
    //height += -tanh(0.02f*abs(dot(pixel,pixel))-0.5);
    //height = clamp(1.7f*height,0.0,0.4);

    // deformed coordinate
    vec3 dPos = normalize(vec3(globalMatrix*vec4(pixel.x,0.0f, pixel.y,1.0f)));

    // Compute normal
    // may convert to non-euclidian space before computing the actual value
    vec2 s = dot(hi-lo,vec2(0.5f))/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    vec2 t1 = pixel - vec2(1.0f,0.0f)*s;
    vec2 t2 = pixel + vec2(1.0f,0.0f)*s;
    vec2 t3 = pixel - vec2(0.0f,1.0f)*s;
    vec2 t4 = pixel + vec2(0.0f,1.0f)*s;


    //vec3 e1 = vec3(t1.x-t2.x,calc_height(t1) - calc_height(t2), t1.y-t2.y);
    //vec3 e2 = vec3(t3.x-t4.x,calc_height(t3) - calc_height(t4), t3.y-t4.y);

    vec3 e1 = convertToSphere(t1) - convertToSphere(t2);
    vec3 e2 = convertToSphere(t3) - convertToSphere(t4);

    vec3 normal = normalize(-cross(e1,e2));
    //normal = normalize(vec3(globalMatrix*vec4(normal,1.0f)));

    //debug
    //float height = EFFECTIVE_HEIGHT*texture(noise,  pixel ).r;

    imageStore(heightmap, p, vec4(height,normal));

}
