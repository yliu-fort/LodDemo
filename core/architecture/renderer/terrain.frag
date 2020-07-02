#version 330 core
out vec4 FragColor;

in float height_display;
in vec2 TexCoords;
in vec3 Normal;

uniform sampler1D colormap;
uniform sampler2D material;
void main()
{
    // Colormap
    //vec3 color = texture(colormap, pow(height_display,0.2f)).rgb;

    // Material
    //vec3 color = texture(material,
    //                     vec2((TexCoords.x + floor(16*height_display))/8.0f ,TexCoords.y)).rgb;
    //if(height_display < 1.0/6e6) { color = vec3(0.2,0.2,0.7); } // ocean

    // Pcolor
    //FragColor = vec4(1.0);

    // Output
    FragColor = vec4(Normal, 1.0f);
}
