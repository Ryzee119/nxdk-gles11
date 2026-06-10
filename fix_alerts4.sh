#!/bin/bash

# Fix the width * bytes_per_pixel overflow in gles_texture.c
sed -i 's/width \* bytes_per_pixel/(size_t)width \* bytes_per_pixel/g' gles_texture.c

# Fix the w and h parameter names in glCopyTexSubImage2D
sed -i 's/GLsizei w, GLsizei h/GLsizei width, GLsizei height/g' gles_texture.c
sed -i 's/(size_t)w \* (size_t)h \* 4/(size_t)width \* (size_t)height \* 4/g' gles_texture.c
sed -i 's/glReadPixels(x, y, w, h, format, type, pixels)/glReadPixels(x, y, width, height, format, type, pixels)/g' gles_texture.c
sed -i 's/glTexSubImage2D(target, level, xoff, yoff, w, h, format, type, pixels)/glTexSubImage2D(target, level, xoff, yoff, width, height, format, type, pixels)/g' gles_texture.c

# Fix the bytes_per_pixel size_t cast
sed -i 's/(size_t)height \* bytes_per_pixel/(size_t)height \* (size_t)bytes_per_pixel/g' gles_texture.c

