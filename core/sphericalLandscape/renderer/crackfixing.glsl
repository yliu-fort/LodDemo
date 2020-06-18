#version 430
#define HEIGHT_MAP_X (17)
#define HEIGHT_MAP_Y (17)
// Kernel
layout(local_size_x = 17, local_size_y = 1, local_size_z = 1) in;

// Child heightmaP
layout(rgba32f, binding = 0) uniform image2D heightmap;

// Parent heightmap
layout(binding = 0) uniform sampler2D heightmap_neighbour;

uniform vec2 mylo; // [0, 0.5, 1]
uniform vec2 myhi; // [0, 0.5, 1]

uniform vec2 shlo; // [0, 0.5, 1]
uniform vec2 shhi; // [0, 0.5, 1]

void main()
{

    // get index in global work group i.e x,y position
    int p = int(gl_LocalInvocationID.x);
    if(p >= HEIGHT_MAP_X) return;

    // map to global texture coordinate
    //vec2 m_p = mylo * vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)
    //        + float(p)/(HEIGHT_MAP_X-1)*(myhi * vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)-mylo * vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)); // [0-15]
    //vec2 sh_p = shlo + (float(p*(shhi-shlo) + 0.5)/(HEIGHT_MAP_X)); // [f1-f2]

    vec2 pixel = mylo * vec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1) + p*(myhi-mylo); // map to lo->hi
    vec2 sh_pixel = ( shlo*ivec2(HEIGHT_MAP_X-1, HEIGHT_MAP_Y-1)
                    + (p*(shhi-shlo)+0.5f) )/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y); // map to shlo->shhi

    //vec2 sh_pixel = shlo + (float(p+0.5)*(shhi-shlo))/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);

    // Black magic (only works when diff(level) = 1)
    //bool offset = (shhi.x-shlo.x > 1e-3 && shlo.x > 0.49) || (shhi.y-shlo.y > 1e-3 && shlo.y > 0.49);
    //sh_pixel += offset? -0.25/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y): 0.25/vec2(HEIGHT_MAP_X, HEIGHT_MAP_Y);

    // sample
    vec4 sh_val = texture(heightmap_neighbour,  sh_pixel );

    imageStore(heightmap, ivec2(pixel), sh_val);

}
