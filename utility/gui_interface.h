#ifndef GUI_INTERFACE_H
#define GUI_INTERFACE_H

// triangle table maps same cube vertex index to a list of up to 5 triangles
// which are built from the interpolated edge vertices
// Isosurface require: GLwindow, Camera initialized
// can produce texture with 1-4 channels
//class GLFWwindow; // cause undefined external symbol.. probably linker issue

namespace GuiInterface {
void Init(void* window);
void Demo();
void ReloadShader();
void Finalize();

// Passing 3d texture
void Begin();
void Draw(void (*func_ptr)(void));
void End();
}

#endif
