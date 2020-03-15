#version 330 core
out vec4 FragColor;

in float height_display;
in vec2 TexCoords;

uniform sampler1D colormap;
uniform sampler2D material;
void main()
{
    vec3 color = texture(colormap, pow(height_display,0.2f)).rgb;
    //vec3 color = texture(material,
    //                     vec2((TexCoords.x + floor(16*height_display))/8.0f ,TexCoords.y)).rgb;
    //if(height_display < 1.0/6e6) { color = vec3(0.2,0.2,0.7); }
    //FragColor = vec4(1.0);
    FragColor = vec4(color, 1.0f);
}
