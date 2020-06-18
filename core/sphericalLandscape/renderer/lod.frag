#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform vec3 viewPos;

void main()
{
    float d = distance(viewPos, FragPos);
    vec3 color[9];

    color[8] = vec3(1);
    color[7] = vec3(1,0,0);
    color[6] = vec3(0,1,0);
    color[5] = vec3(0,0,1);
    color[4] = vec3(1,1,0);
    color[3] = vec3(1,0,1);
    color[2] = vec3(0,0,1);
    color[1] = vec3(0.2,0.7,0.4);
    color[0] = vec3(0.5);

    int i = 0;
    for(i = 0; i < 9; i++)
    {
        d *= 2.0f;
        if(d > 2.8f) {break;}
    }

    FragColor = vec4(color[i],1.0);
}
