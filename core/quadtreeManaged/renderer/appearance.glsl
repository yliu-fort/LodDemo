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
//layout(binding = 1) uniform sampler2D noise;
layout(binding = 1) uniform samplerCube elevationmap;
layout(binding = 2) uniform sampler2DArray material;

//uniform vec2 lo;
//uniform vec2 hi;

uniform mat4 globalMatrix;

uniform int level;
uniform int hash;

vec2 dpos(int code)
{
    vec2 o = vec2(-1);
    for(int i = 0; i < 15; i++)
    {
        o += vec2((code>>1)&1, (code)&1)/float(1<<i);
        code >>= 2;
    }
    return o;
}

vec2 getCurrentUV()
{
    return dpos(hash)
            + 2.0*ivec2(gl_GlobalInvocationID.xy)
            /vec2(ALBEDO_MAP_X-1, ALBEDO_MAP_Y-1)
            /float(1<<level);
}

//const float freq[13] = {4.03*32,1.96*32,1.01*32, 32.0f/2.03f,
//                 32.0f/3.98f, 32.0f/8.01f, 32.0f/15.97f,
//                 32.0f/31.98f,32.0f/64.01f,32.0f/128.97f,
//                 32.0f/256.07f,32.0f/511.89f,32.0f/1024.22f};
//const int freqindex[16] = {0,1,1,2,
//                       0,2,1,0,
//                       2,1,2,2,
//                       0,0,2,1};
float snoise(vec2 v);

float ridgenoise(vec2 t, int freq) {
  return  2.0*(0.5 - abs( 0.5 - snoise( t*(1<<freq)*16.00 ) ));
}

#define EFFECTIVE_HEIGHT_SYNTHETIC (0.001)
#define EFFECTIVE_HEIGHT (0.005)
float calc_height(vec2 pixel);

// Always read north face
vec3 convertToGlobal(vec2 t)
{
    return vec3(globalMatrix*vec4(t,1.0f,1.0f));
}


float calc_height(vec2 pixel)
{

    // Noise sampler1D
    float density = ridgenoise( pixel,0 );

    for(int i = 1; i < 8; i++)
    {
        density += ridgenoise( pixel,i ) * density / float(1<<i);
    }

    density /= 2.0;
    // Procedure
    density = EFFECTIVE_HEIGHT_SYNTHETIC*clamp(density,0.0,1.0);

    // Read elevation map
    // Caution: low-res elevation map causes bumpy ground-> truncation issue
    //density += -2.0*tanh(0.03f*abs(dot(pixel,pixel))-0.15);
    //vec2 offset = 0.5/vec2(ELEVATION_MAP_RESOLUTION);
    //vec2 cpixel = offset + pixel*(1.0f - 2.0f*offset);

    density += texture( elevationmap,  vec3(convertToGlobal(pixel)) ).r;

    // Bound height
    density = clamp(density,0.0,EFFECTIVE_HEIGHT);

    return density;
}




void main()
{

    // get index in global work group i.e x,y position
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if(p.x >= ALBEDO_MAP_X || p.y >= ALBEDO_MAP_Y) return;

    vec2 pixel = getCurrentUV();

    // map to global texture coordinate
    //pixel = lo + ( pixel )*(hi-lo); // [lo, hi]
    //pixel = dpos(hash) + 2.0*pixel/double(1<<level);

    // Procedure
    float height = calc_height(pixel);

    vec3 color = mix(textureLod( material,
                     vec3(pixel*(1<<15), 6.45f*height/EFFECTIVE_HEIGHT), 15-level ).rgb,
                vec3(0.2,0.2,0.7), clamp(tanh(5e-4f/(50*height+5e-5f))-0.1f,0,1));

    imageStore(albedo, p, vec4(color,1.0f));

}

//
// Description : Array and textureless GLSL 2D simplex noise function.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20110822 (ijm)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
//

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec2 mod289(vec2 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
  return mod289(((x*34.0)+1.0)*x);
}

float snoise(vec2 v)
  {
  const vec4 C = vec4(0.211324865405187,  // (3.0-sqrt(3.0))/6.0
                      0.366025403784439,  // 0.5*(sqrt(3.0)-1.0)
                     -0.577350269189626,  // -1.0 + 2.0 * C.x
                      0.024390243902439); // 1.0 / 41.0
// First corner
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);

// Other corners
  vec2 i1;
  //i1.x = step( x0.y, x0.x ); // x0.x > x0.y ? 1.0 : 0.0
  //i1.y = 1.0 - i1.x;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  // x0 = x0 - 0.0 + 0.0 * C.xx ;
  // x1 = x0 - i1 + 1.0 * C.xx ;
  // x2 = x0 - 1.0 + 2.0 * C.xx ;
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;

// Permutations
  i = mod289(i); // Avoid truncation effects in permutation
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
                + i.x + vec3(0.0, i1.x, 1.0 ));

  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;

// Gradients: 41 points uniformly over a line, mapped onto a diamond.
// The ring size 17*17 = 289 is close to a multiple of 41 (41*7 = 287)

  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;

// Normalise gradients implicitly by scaling m
// Approximation of: m *= inversesqrt( a0*a0 + h*h );
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );

// Compute final noise value at P
  vec3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}
