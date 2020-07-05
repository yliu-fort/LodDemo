#include "UCartesianMath.h"
#include <cmath>

using namespace glm;
using uint = uint32;

namespace Umath
{


uint ExpandBits2(uint v)
{
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}


uint ExpandBits1(uint v)
{
    v = (v ^ (v << 8) ) & 0x00FF00FFu;
    v = (v ^ (v << 4) ) & 0x0F0F0F0Fu;
    v = (v ^ (v << 2) ) & 0x33333333u;
    v = (v ^ (v << 1) ) & 0x55555555u;
    return v;
}


uint MergeBits1(uint v)
{
    v &= 0x55555555u;
    v = (v ^ (v >> 1 )) & 0x33333333u;
    v = (v ^ (v >> 2 )) & 0x0F0F0F0Fu;
    v = (v ^ (v >> 4 )) & 0x00FF00FFu;
    v = (v ^ (v >> 8 )) & 0x0000FFFFu;
    return v;
}


uint MergeBits2(uint v)
{
    v &= 0x09249249u;
    v = (v ^ (v >> 2 )) & 0x030C30C3u;
    v = (v ^ (v >> 4 )) & 0x0300F00Fu;
    v = (v ^ (v >> 8 )) & 0xFF0000FFu;
    v = (v ^ (v >> 16)) & 0x000003FFu;
    return v;
}


uint EncodeMorton3(vec3 n)
{
    n[0] = min(max(n[0] * 1024.0f, 0.0f), 1023.0f);
    n[1] = min(max(n[1] * 1024.0f, 0.0f), 1023.0f);
    n[2] = min(max(n[2] * 1024.0f, 0.0f), 1023.0f);
    uint xx = ExpandBits2(uint(n[0]));
    uint yy = ExpandBits2(uint(n[1]));
    uint zz = ExpandBits2(uint(n[2]));
    return (xx << 2) | (yy << 1) | (zz << 0);
}


uint EncodeMorton3(float x, float y, float z)
{
    x = min(max(x * 1024.0f, 0.0f), 1023.0f);
    y = min(max(y * 1024.0f, 0.0f), 1023.0f);
    z = min(max(z * 1024.0f, 0.0f), 1023.0f);
    uint xx = ExpandBits2(uint(x));
    uint yy = ExpandBits2(uint(y));
    uint zz = ExpandBits2(uint(z));
    return (xx << 2) | (yy << 1) | (zz << 0);
}


uint EncodeMorton2(vec2 n)
{
    n[0] = min(max(n[0] * 65536.0f, 0.0f), 65535.0f);
    n[1] = min(max(n[1] * 65536.0f, 0.0f), 65535.0f);
    uint xx = ExpandBits1(uint(n[0]));
    uint yy = ExpandBits1(uint(n[1]));
    return (xx << 1) | (yy << 0);
}


uint EncodeMorton2(float x, float y)
{
    x = min(max(x * 65536.0f, 0.0f), 65535.0f);
    y = min(max(y * 65536.0f, 0.0f), 65535.0f);
    uint xx = ExpandBits1(uint(x));
    uint yy = ExpandBits1(uint(y));
    return (xx << 1) | (yy << 0);
}


vec3 DecodeMorton3(uint v)
{
    return vec3(
                float(MergeBits2(v >> 2 )) / 1024.0f,
                float(MergeBits2(v >> 1 )) / 1024.0f,
                float(MergeBits2(v >> 0 )) / 1024.0f
                );
}


vec2 DecodeMorton2(uint v)
{
    vec2 n(
                float(MergeBits1(v >> 1 )) / 65536.0f,
                float(MergeBits1(v >> 0 )) / 65536.0f
                );

    return n;
}

// Highest level is 10, lowest level (root) is 0
uint EncodeMortonWithLod3(vec3 n, uint l)
{
    //assert(l < 10, "Umath::EncodeMortonWithLod3: Fatal Error: Out of available lod range.");
    return (Umath::EncodeMorton3(n) | 0x80000000u) >> (3*l);
}


uint EncodeMortonWithLod3(float x, float y, float z, uint l)
{
    return (Umath::EncodeMorton3(x, y, z) | 0x80000000u) >> (3*l);
}

// Highest level is 15, lowest level (root) is 0
uint EncodeMortonWithLod2(vec2 n, uint l)
{
    return ((Umath::EncodeMorton2(n) >> 2) | 0x80000000u) >> (2*l);
}


uint EncodeMortonWithLod2(float x, float y, uint l)
{
    return ((Umath::EncodeMorton2(x, y) >> 2) | 0x80000000u) >> (2*l);
}


vec3 DecodeMortonWithLod3(uint v, uint l)
{
    v <<= (3*l);
    v &= 0x3FFFFFFFu;
    return DecodeMorton3(v);
}


vec2 DecodeMortonWithLod2(uint v, uint l)
{
    v <<= (2*l);
    v &= 0x3FFFFFFFu;
    v <<= 2;
    return DecodeMorton2(v);
}

// for validation purpose
uint CountLeadingZeros(uint x)
{
    //const int numIntBits = sizeof(uint) * 8; //compile time constant
    const int numIntBits = 32;
    //do the smearing
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    //count the ones
    x -= x >> 1 & 0x55555555;
    x = (x >> 2 & 0x33333333) + (x & 0x33333333);
    x = (x >> 4) + x & 0x0f0f0f0f;
    x += x >> 8;
    x += x >> 16;
    return numIntBits - (x & 0x0000003f); //subtract # of 1s from 32
}

uint GetLodLevel(uint x)
{
    return CountLeadingZeros(x)/2;
}

}
