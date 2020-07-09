#include "UCartesianMath.h"
#include <cmath>
#include <iostream>
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
    return (xx << 0) | (yy << 1) | (zz << 2);
}


uint EncodeMorton3(float x, float y, float z)
{
    x = min(max(x * 1024.0f, 0.0f), 1023.0f);
    y = min(max(y * 1024.0f, 0.0f), 1023.0f);
    z = min(max(z * 1024.0f, 0.0f), 1023.0f);
    uint xx = ExpandBits2(uint(x));
    uint yy = ExpandBits2(uint(y));
    uint zz = ExpandBits2(uint(z));
    return (xx << 0) | (yy << 1) | (zz << 2);
}


uint EncodeMorton2(vec2 n)
{
    n[0] = min(max(n[0] * 65536.0f, 0.0f), 65535.0f);
    n[1] = min(max(n[1] * 65536.0f, 0.0f), 65535.0f);
    uint xx = ExpandBits1(uint(n[0]));
    uint yy = ExpandBits1(uint(n[1]));
    return (xx << 0) | (yy << 1);
}


uint EncodeMorton2(float x, float y)
{
    x = min(max(x * 65536.0f, 0.0f), 65535.0f);
    y = min(max(y * 65536.0f, 0.0f), 65535.0f);
    uint xx = ExpandBits1(uint(x));
    uint yy = ExpandBits1(uint(y));
    return (xx << 0) | (yy << 1);
}


vec3 DecodeMorton3(uint v)
{
    return vec3(
                float(MergeBits2(v >> 0 )) / 1024.0f,
                float(MergeBits2(v >> 1 )) / 1024.0f,
                float(MergeBits2(v >> 2 )) / 1024.0f
                );
}


vec2 DecodeMorton2(uint v)
{
    vec2 n(
                float(MergeBits1(v >> 0 )) / 65536.0f,
                float(MergeBits1(v >> 1 )) / 65536.0f
                );

    return n;
}

// Helper functions
uint ReshapeMortonAddLod3(const uint& v, const uint& l)
{
    return (v | 0x80000000u) >> (3*l);
}

uint ReshapeMortonAddLod2(const uint& v, const uint& l)
{
    return ((v >> 2) | 0x80000000u) >> (2*l);
}

uint ReshapeMortonDiscardLod3(uint v, const uint& l)
{
    v <<= (3*l);
    v &= 0x3FFFFFFFu;

    return v;
}

uint ReshapeMortonDiscardLod2(uint v, const uint& l)
{
    v <<= (2*l);
    v &= 0x3FFFFFFFu;
    v <<= 2;

    return v;
}

// Highest level is 10, lowest level (root) is 0
uint EncodeMortonWithLod3(vec3 n, uint l)
{
    //assert(l < 10, "Umath::EncodeMortonWithLod3: Fatal Error: Out of available lod range.");
    return ReshapeMortonAddLod3(Umath::EncodeMorton3(n), l);
}

uint EncodeMortonWithLod3(float x, float y, float z, uint l)
{
    return ReshapeMortonAddLod3(Umath::EncodeMorton3(x, y, z),l);
}

// Highest level is 15, lowest level (root) is 0
uint EncodeMortonWithLod2(vec2 n, uint l)
{
    return ReshapeMortonAddLod2(Umath::EncodeMorton2(n),l);
}


uint EncodeMortonWithLod2(float x, float y, uint l)
{
    return ReshapeMortonAddLod2(Umath::EncodeMorton2(x,y),l);
}

vec3 DecodeMortonWithLod3(uint v, uint l)
{
    return DecodeMorton3(ReshapeMortonDiscardLod3(v,l));
}

vec2 DecodeMortonWithLod2(uint v, uint l)
{
    return DecodeMorton2(ReshapeMortonDiscardLod2(v,l));
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

// Return 0xFFFFFFFF when out of range
// ox, oy must have form like 1 << n for integer lattice move
uint GetNeighbourWithLod2(uint v, int ox, int oy, uint l)
{
    v = ReshapeMortonDiscardLod2(v,l);

    // sign preserve Lsh
    ox <<= (l+1);
    oy <<= (l+1);

    ivec2 n(
                (int)MergeBits1(v >> 0 ) + ox,
                (int)MergeBits1(v >> 1 ) + oy
                );
    //std::cout << n.x << ", " << n.y << std::endl;
    if(n.x < 0 || n.x > 65535 || n.y < 0 || n.y > 65535)
        return 0xFFFFFFFFu;

    return EncodeMortonWithLod2(vec2(n)/65536.0f, l);
}

// diagonal mirror v *= 3FFF FFFF or FFFF FFFF
uint FlipDiagWithLod2(uint v, uint l)
{
    v = ReshapeMortonDiscardLod2(v,l);
    v ^= 0xFFFFFFFFu;
    return ReshapeMortonAddLod2(v,l);
}

// horizontal mirror v*= 1555 5555 or 5555 5555
uint FlipLRWithLod2(uint v, uint l)
{
    v = ReshapeMortonDiscardLod2(v,l);
    v ^= 0x55555555u;
    return ReshapeMortonAddLod2(v,l);
}

// vertical mirror v*= 2AAA AAAA or AAAA AAAA
uint FlipUDWithLod2(uint v, uint l)
{
    v = ReshapeMortonDiscardLod2(v,l);
    v ^= 0xAAAAAAAAu;
    return ReshapeMortonAddLod2(v,l);
}

uint FlipWithLod2(uint v, int ox, int oy, uint l)
{
    if(ox == 0 && oy == 0) return v;
    if(ox == 0) return FlipUDWithLod2(v,l);
    if(oy == 0) return FlipLRWithLod2(v,l);
    return FlipDiagWithLod2(v,l);

}

}
