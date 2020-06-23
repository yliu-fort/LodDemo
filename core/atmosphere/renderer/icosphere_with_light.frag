#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{

    // get diffuse color
    vec3 color = texture(diffuseMap, TexCoords).rgb;

    // ambient
    vec3 ambient = 0.1 * color;
    // diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, Normal), 0.0);
    vec3 diffuse = diff * color * 2.0f;
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, Normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 32.0);

    vec3 specular = 0.2 * spec * texture(specularMap, TexCoords).rgb;
    FragColor = vec4(ambient + diffuse + specular, 1.0);

    //FragColor = vec4(normal, 1.0f);

}
