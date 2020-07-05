#version 430
#define HEIGHT_MAP_X (19)
#define HEIGHT_MAP_Y (19)
#define ELEVATION_MAP_RESOLUTION (256)
// Kernel
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Child heightmaP
layout(rgba32f, binding = 0) uniform image2D heightmap;

// Parent heightmap
//layout(binding = 0) uniform sampler2D heightmap_parent;

// noisemap
//layout(binding = 1) uniform sampler2D noise;
layout(binding = 1) uniform samplerCube elevationmap;

//uniform vec2 lo;
//uniform vec2 hi;

uniform mat4 globalMatrix;

uniform int level;
uniform int hash;

int MergeBits1(int v)
{
    v &= 0x55555555;
    v = (v ^ (v >> 1 )) & 0x33333333;
    v = (v ^ (v >> 2 )) & 0x0F0F0F0F;
    v = (v ^ (v >> 4 )) & 0x00FF00FF;
    v = (v ^ (v >> 8 )) & 0x0000FFFF;
    return v;
}

vec2 DecodeMortonWithLod2(int v, int l)
{
    v <<= (2*l);
    v &= 0x3FFFFFFF;
    v <<= 2;
    return vec2(
                float(MergeBits1(v >> 0 )) / 65536.0f,
                float(MergeBits1(v >> 1 )) / 65536.0f
                );
}

vec2 getCurrentUV()
{
    return 2.0f*DecodeMortonWithLod2(hash, 15-level)-1.0f
            + 2.0*ivec2(gl_GlobalInvocationID.xy)
            /vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)
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
float snoise(vec3 v);

float ridgenoise(vec2 t, int freq) {
  return  2.0*(0.5 - abs( 0.5 - snoise( t*(1<<freq)*16.0 ) ));
}

#define EFFECTIVE_HEIGHT_SYNTHETIC (0.001)
#define EFFECTIVE_HEIGHT (0.002)

vec3 convertToDeformed(vec2 t)
{
    return vec3(globalMatrix*vec4(t.x,0.0f,t.y,1.0f));
}


vec3 convertToSphere(vec2 t)
{
    return normalize(convertToDeformed(t));
}

vec2 convertToRadial(vec3 coord)
{
    return vec2(atan(coord.y,coord.x),acos(coord.z));
}

// Turbulence-like pattern
//float ridgenoises3(vec2 t, int freq) {
//    vec3 v = convertToSphere(t);
//  return  2.0*(0.5 - abs( 0.5 - snoise( vec2(snoise( v.xy*(1<<freq)*16 ), v.z ) )));
//}

// S3 noise
//float ridgenoises3(vec2 t, int freq) {
//    vec3 v = convertToSphere(t);
//
//    return  2.0*(0.5 - abs( 0.5 -
//                            mix(
//                                mix(
//                                    snoise( vec2(snoise( v.xy*(1<<freq)*16.0f ), v.z ) )
//                                    ,snoise( vec2(snoise( v.xz*(1<<freq)*16.0f ), v.y ) )
//                                    ,abs(v.y))
//                                ,snoise( vec2(snoise( v.yz*(1<<freq)*16.0f ), v.x ) )
//                                ,abs(v.x)) ));
//
//}

float ridgenoises3(vec2 t, int freq) {
    vec3 v = convertToDeformed(t);
    return  2.0*(0.5 - abs( 0.5 - snoise( v*(1<<freq)*64.0f ) ) );
}


float calc_height(vec2 pixel)
{

    // Noise sampler1D
    float density = ridgenoises3( pixel,0 );

    for(int i = 1; i < 8; i++)
    {
        density += ridgenoises3( pixel,i + 2 ) * density / float(1<<i);
    }

    density /= 2.0;
    // Procedure
    density = EFFECTIVE_HEIGHT_SYNTHETIC*clamp(density,0.0,1.0);


    // Read elevation map
    // Caution: low-res elevation map causes bumpy ground-> truncation issue
    //density += -2.0*tanh(0.03f*abs(dot(pixel,pixel))-0.15);
    //vec2 offset = 0.5/vec2(ELEVATION_MAP_RESOLUTION);
    //vec2 cpixel = offset + pixel*(1.0f - 2.0f*offset);

    float heightData = 2.0f*(texture( elevationmap,  vec3(convertToSphere(pixel)) ).r - 0.5f);
    density = mix(0, density, 2*heightData);
    density += EFFECTIVE_HEIGHT*heightData;

    // Bound height
    density = clamp(density,0.0f,EFFECTIVE_HEIGHT);

    return density;
}



void main()
{

    // get index in global work group i.e x,y position
    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if(p.x >= HEIGHT_MAP_X || p.y >= HEIGHT_MAP_Y) return;

    //vec2 pixel = vec2(p/vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)); // map [0,1]

    // map to global texture coordinate
    //pixel = lo + ( pixel )*(hi-lo); // [lo, hi]
    vec2 pixel = getCurrentUV();

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
    //vec3 dPos = normalize(vec3(globalMatrix*vec4(pixel.x,0.0f, pixel.y,1.0f)));

    // Compute normal
    // may convert to non-euclidian space before computing the actual value
    vec2 s = (1.0/(1<<level))/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);
    vec2 t1 = vec2(pixel) - vec2(1.0f,0.0f)*s;
    vec2 t2 = vec2(pixel) + vec2(1.0f,0.0f)*s;
    vec2 t3 = vec2(pixel) - vec2(0.0f,1.0f)*s;
    vec2 t4 = vec2(pixel) + vec2(0.0f,1.0f)*s;


    //vec3 e1 = vec3(-2.0f*s.x, calc_height(t1) - calc_height(t2), 0);
    //vec3 e2 = vec3(0, calc_height(t3) - calc_height(t4), -2.0f*s.y);

    //vec3 e1 = (1.0 + calc_height(t1))*convertToSphere(t1) - (1.0 + calc_height(t2))*convertToSphere(t2);
    //vec3 e2 = (1.0 + calc_height(t3))*convertToSphere(t3) - (1.0 + calc_height(t4))*convertToSphere(t4);

    //vec3 normal = normalize(-cross(e1,e2));
    //normal = normalize(vec3(globalMatrix*vec4(normal,1.0f)));

    // Compute tangent
    vec3 tangent = normalize( -convertToSphere(t1) + convertToSphere(t2));

    //debug
    //float height = EFFECTIVE_HEIGHT*texture(noise,  pixel ).r;

    imageStore(heightmap, p, vec4(height,tangent));

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

vec2 mod289(vec2 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 mod289(vec3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
    return mod289(((x*34.0)+1.0)*x);
}

vec4 permute(vec4 x) {
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

vec4 taylorInvSqrt(vec4 r)
{
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
  {
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //   x0 = x0 - 0.0 + 0.0 * C.xxx;
  //   x1 = x0 - i1  + 1.0 * C.xxx;
  //   x2 = x0 - i2  + 2.0 * C.xxx;
  //   x3 = x0 - 1.0 + 3.0 * C.xxx;
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
  i = mod289(i);
  vec4 p = permute( permute( permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 ))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
  float n_ = 0.142857142857; // 1.0/7.0
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
  //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1),
                                dot(p2,x2), dot(p3,x3) ) );
  }
