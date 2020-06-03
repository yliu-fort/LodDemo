#version 330 core
out vec4 FragColor;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define NR_POINT_LIGHTS 4

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;
in float height_display;
in float blend_display;

uniform sampler1D colormap;
uniform sampler2D material;
uniform sampler2D debugmap;
uniform int render_type;
uniform int level;

uniform vec3 viewPos;
uniform DirLight dirLight;
//uniform PointLight pointLights[NR_POINT_LIGHTS];

// function prototypes
vec3 CalcDirLight(DirLight, vec3, vec3, vec3);
//vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    // properties
    //vec3 norm = Normal;
    //vec3 norm = texture(texture_normal1, TexCoords).rgb;
    //norm = normalize(norm * 2.0f - 1.0f);
    //norm = normalize(TBN * norm);

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 color;

    if(render_type == 0) // REAL
    {
        color = mix(texture( material,
                         vec2((TexCoords.x + floor(135*height_display))/8.0f ,TexCoords.y) ).rgb,
                    vec3(0.2,0.2,0.7), clamp(tanh(5e-4f/(height_display+5e-5f))-0.1f,0,1));

        color = CalcDirLight(dirLight, Normal, viewDir, color);
    }else
    if(render_type == 1) // COLORMAP
    {
        //color = texture(colormap, pow(FragPos.y,0.2f)).rgb;
        color = mix(texture( debugmap, TexCoords*level ).rgb, vec3(0.0,0.0,1.0),blend_display);
        if(blend_display == 1.0)
            color = vec3(0.0,0.0,0.0);
        if(blend_display == 0.0)
            color = vec3(1.0,1.0,1.0);
    }else
    if(render_type == 2) // NORMAL
    {
        color = Normal;
    }else
    if(render_type == 3) // PCOLOR
    {
        //color = mix(texture( debugmap, TexCoords*level ).rgb, vec3(0.0,0.0,1.0),blend_display);
        //if(blend_display == 1.0)
        //    color = vec3(0.0,0.0,0.0);
        //color = texture( debugmap, TexCoords*level ).rgb;
        color = CalcDirLight(dirLight, Normal, viewDir, vec3(0.7,0.7,0.7));

    }

    // Output color
    FragColor = vec4(color, 1.0);

}

// calculates the color when using a directional light.
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir, vec3 color)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 4.0);
    // combine results
    vec3 ambient = light.ambient * color;
    vec3 diffuse = light.diffuse * diff * color;
    vec3 specular = light.specular * spec * color;
    return (ambient + diffuse + specular);
}


