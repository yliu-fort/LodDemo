#pragma once
#include <iostream>
#include <vector>
#include <glad/glad.h>

//Using Sam Leffler's libtiff library
unsigned int loadTiffTexture(const char *path, const std::string &directory, bool gamma);


unsigned int loadTexture(const char *path, const std::string &directory, bool gamma);

unsigned int loadLayeredTexture(const char *path, const std::string &directory, bool gamma);

unsigned int loadCubemap(std::vector<std::string> faces, GLenum texType=GL_RGB, GLenum dataType=GL_UNSIGNED_BYTE);
