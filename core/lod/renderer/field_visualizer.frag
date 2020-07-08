#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D f0;

#define NG (2)
#define FX (28 + 2*NG)
#define FY (28 + 2*NG)

void main()
{
    // align with the edge of first cell center, border is fully invisible
    vec3 color = texture(f0, (TexCoords*vec2(FX - 3*NG,FY - 3*NG) + 1.5*NG)/vec2(FX,FY) ).rgb;

    // align with the edge of first vertex, border is partially invisible
    //vec3 color = texture(f0, (TexCoords*vec2(FX - 2*NG,FY - 2*NG) + 1.0*NG)/vec2(FX,FY) ).rgb;

    // can see border colors (red and blue)
    //vec3 color = texture(f0, TexCoords ).rgb;

    FragColor = vec4(color, 1.0);
}
