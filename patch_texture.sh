#!/bin/bash

# Remove assert and void casts from glCopyTexImage2D
sed -i '1080,1088c\
    if (internalformat != GL_RGB && internalformat != GL_RGBA) {\n\
        gliSetError(GL_INVALID_ENUM);\n\
        return;\n\
    }\n\
    GLenum format = internalformat;\n\
    GLenum type = GL_UNSIGNED_BYTE;\n\
    GLsizei bytes_per_pixel = (format == GL_RGBA) ? 4 : 3;\n\
    void *pixels = GLI_MALLOC(width * height * bytes_per_pixel);\n\
    if (!pixels) {\n\
        gliSetError(GL_OUT_OF_MEMORY);\n\
        return;\n\
    }\n\
    glReadPixels(x, y, width, height, format, type, pixels);\n\
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);\n\
    GLI_FREE(pixels);\n\
' gles_texture.c

# Remove assert and void casts from glCopyTexSubImage2D
sed -i '1095,1104c\
    GLenum format = GL_RGBA;\n\
    GLenum type = GL_UNSIGNED_BYTE;\n\
    void *pixels = GLI_MALLOC(w * h * 4);\n\
    if (!pixels) {\n\
        gliSetError(GL_OUT_OF_MEMORY);\n\
        return;\n\
    }\n\
    glReadPixels(x, y, w, h, format, type, pixels);\n\
    glTexSubImage2D(target, level, xoff, yoff, w, h, format, type, pixels);\n\
    GLI_FREE(pixels);\n\
' gles_texture.c
