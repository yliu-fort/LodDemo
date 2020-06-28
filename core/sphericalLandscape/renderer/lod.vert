#version 330 core
#define HEIGHT_MAP_X (17)
#define HEIGHT_MAP_Y (17)
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;

out VS_OUT {
    vec3 normal;
} vs_out;

uniform mat4 m4ModelViewProjectionMatrix;
uniform mat4 model;
uniform sampler2D heightmap;
uniform sampler2D heightmapParent;
void main()
{

    // Texture coordinate
    TexCoords = aTexCoords;

    // Heightmap
    float height = texture(heightmap, aTexCoords).r;

    vs_out.normal = texture(heightmap, aTexCoords).gba;

    // Modify fragpos
    FragPos = vec3(model*vec4(aPos.x,aPos.y+height,aPos.z,1.0));

    gl_Position = m4ModelViewProjectionMatrix * vec4(FragPos, 1.0);
}
