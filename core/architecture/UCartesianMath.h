#include "glm/glm.hpp"

namespace Umath
{

// by inserting 2 zeros after each bit.
glm::uint32 ExpandBits2(glm::uint32);

// by inserting 1 zeros after each bit.
glm::uint32 ExpandBits1(glm::uint32);

// merges bits interleaved by 2 zeros.
glm::uint32 MergeBits2(glm::uint32);

// merges bits interleaved by 1 zeros.
glm::uint32 MergeBits1(glm::uint32);

// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
glm::uint32 EncodeMorton3(glm::vec3);
glm::uint32 EncodeMorton3(float, float, float);

// Calculates a 32-bit Morton code for the
// given 2D point located within the unit rect [0,1].
glm::uint32 EncodeMorton2(glm::vec2);
glm::uint32 EncodeMorton2(float, float);

// Decodes Morton code by calling mergebits method.
glm::vec3 DecodeMorton3(glm::uint32);
glm::vec2 DecodeMorton2(glm::uint32);


// Encodes morton code for multi-scale objects
// 0-10 lod level is available for 3d morton
// 0-15 lod level is available for 2d morton
glm::uint32 EncodeMortonWithLod3(glm::vec3, glm::uint32);
glm::uint32 EncodeMortonWithLod3(float, float, float, glm::uint32);
glm::uint32 EncodeMortonWithLod2(glm::vec2, glm::uint32);
glm::uint32 EncodeMortonWithLod2(float, float, glm::uint32);

glm::vec3 DecodeMortonWithLod3(glm::uint32, glm::uint32);
glm::vec2 DecodeMortonWithLod2(glm::uint32, glm::uint32);

glm::uint32 GetLodLevel(glm::uint32);

glm::uint32 CountLeadingZeros(glm::uint32);
}
