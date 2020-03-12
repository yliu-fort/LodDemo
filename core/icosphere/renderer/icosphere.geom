#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

out vec2 TexCoords;

vec2 texCoords[3];

in VS_OUT {
    vec2 TexCoords;
} gs_in[];

void render_tri()
{
    gl_Position = gl_in[0].gl_Position;
    TexCoords = texCoords[0];
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    TexCoords = texCoords[1];
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    TexCoords = texCoords[2];
    EmitVertex();

    EndPrimitive();
}

void main() {

    texCoords[0] = gs_in[0].TexCoords;
    texCoords[1] = gs_in[1].TexCoords;
    texCoords[2] = gs_in[2].TexCoords;

    //float dist1 = texCoords[0].x - texCoords[1].x;
    //if(abs(dist1) > 0.5f)
    //{
    //    if(dist1 > 0) { texCoords[1].x += 1; }
    //    else {texCoords[0].x += 1;}
    //}
    //
    //float dist2 = texCoords[1].x - texCoords[2].x ;
    //if(abs(dist2) > 0.5f)
    //{
    //    if(dist2 > 0) { texCoords[2].x += 1; }
    //    else {texCoords[1].x += 1;}
    //}
    //
    //float dist3 = texCoords[0].x - texCoords[2].x ;
    //if(abs(dist3) > 0.5f)
    //{
    //    if(dist3 > 0) { texCoords[2].x += 1; }
    //    else {texCoords[0].x += 1;}
    //}
    render_tri();

}
