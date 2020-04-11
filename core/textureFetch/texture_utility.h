#ifndef TEXTURE_UTILITY_H
#define TEXTURE_UTILITY_H
#include <iostream>
#include <vector>

#include <glad/glad.h>

#include "stb_image.h"

#include "tiffio.h"

/*Using Sam Leffler's libtiff library */
unsigned int loadTiffTexture(const char *path, const std::string &directory, bool gamma){
    TIFFRGBAImage img;
    uint32 *raster;

    size_t npixels;
    int imgwidth, imgheight, imgcomponents;

    int hasABGR = 0;
    char *imgfilename  = NULL;

    TIFF *tif;
    char emsg[1024];

    std::string filename = std::string(path);
    filename = directory + '/' + filename;
    const char* name = filename.c_str();

    unsigned int textureID;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Open tiff context
    tif = TIFFOpen(name, "r");
    if (tif == NULL){
        fprintf(stderr, "tif == NULL\n");
        exit(1);
    }

    // Set orientation
    //1 = The 0th row represents the visual top of the image, and the 0th column represents the visual left-hand side.
    //2 = The 0th row represents the visual top of the image, and the 0th column represents the visual right-hand side.
    //3 = The 0th row represents the visual bottom of the image, and the 0th column represents the visual right-hand side.
    //4 = The 0th row represents the visual bottom of the image, and the 0th column represents the visual left-hand side.
    //5 = The 0th row represents the visual left-hand side of the image, and the 0th column represents the visual top.
    //6 = The 0th row represents the visual right-hand side of the image, and the 0th column represents the visual top.
    //7 = The 0th row represents the visual right-hand side of the image, and the 0th column represents the visual bottom.
    //8 = The 0th row represents the visual left-hand side of the image, and the 0th column represents the visual bottom.
    TIFFSetField(tif, TIFFTAG_ORIENTATION, 4);

    // Read image
    if (TIFFRGBAImageBegin(&img, tif, 0,emsg)){

        npixels = img.width*img.height;
        raster = (uint32 *)_TIFFmalloc(npixels*sizeof(uint32));
        if (raster != NULL){
            if (TIFFRGBAImageGet(&img, raster, img.width, img.height) == 0){
                TIFFError(name, emsg);
                exit(1);
            }
        }
        TIFFRGBAImageEnd(&img);
        fprintf(stderr, "Read image %s (%d x %d)\n", name, img.width, img.height);
    }
    else {
        TIFFError(name, emsg);
        exit(1);
    }
    imgwidth = img.width;
    imgheight = img.height;

    // code based upon
    // http://www.opengl.org/developers/code/mjktips/libtiff/showtiff.c

    /* If cannot directly display ABGR format, we need to reverse the component
     ordering in each pixel.  :-(  */
    //if (!hasABGR) {
    //    int i;
    //
    //    for (i = 0; i < npixels; i++) {
    //        register unsigned char *cp = (unsigned char *) &raster[i];
    //        int t;
    //
    //        t = cp[3];
    //        cp[3] = cp[0];
    //        cp[0] = t;
    //        t = cp[2];
    //        cp[2] = cp[1];
    //        cp[1] = t;
    //    }
    //}

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 imgwidth, imgheight,0, GL_RGBA, GL_UNSIGNED_BYTE,raster);

    TIFFClose(tif);
    _TIFFfree(raster);

    return textureID;
}


unsigned int loadTexture(const char *path, const std::string &directory, bool gamma)
{
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(std::vector<std::string> faces, GLenum texType=GL_RGB, GLenum dataType=GL_UNSIGNED_BYTE)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, texType, dataType, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

#endif
