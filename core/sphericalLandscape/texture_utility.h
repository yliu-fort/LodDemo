#pragma once
#include <iostream>
#include <vector>
#include <glad/glad.h>

//Using Sam Leffler's libtiff library
unsigned int loadTiffTexture(const char *path, const std::string &directory, bool gamma);


unsigned int loadTexture(const char *path, const std::string &directory, bool gamma);

unsigned int loadLayeredTexture(const char *path, const std::string &directory, bool gamma);

unsigned int loadCubemap(std::vector<std::string> faces, GLenum texType=GL_RGB, GLenum dataType=GL_UNSIGNED_BYTE);

// Load a large cubemap texture from tiles
unsigned int loadCubemapLarge(std::string path, std::string extension=".jpg", unsigned int lodLevel=0, int baseResolution=256, GLenum texType=GL_RGB, GLenum dataType=GL_UNSIGNED_BYTE);
