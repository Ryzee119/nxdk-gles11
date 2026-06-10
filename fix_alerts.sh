#!/bin/bash

# Fix the integer multiplications in gles_texture.c to prevent overflow
sed -i 's/void \*pixels = GLI_MALLOC(width \* height \* bytes_per_pixel);/void \*pixels = GLI_MALLOC((size_t)width \* (size_t)height \* bytes_per_pixel);/' gles_texture.c
sed -i 's/void \*pixels = GLI_MALLOC(w \* h \* 4);/void \*pixels = GLI_MALLOC((size_t)w \* (size_t)h \* 4);/' gles_texture.c

# Also fixing existing multiplication overflows reported by CodeQL
sed -i 's/rgb_to_rgba_opaque(src_pixels, converted_pixels, width \* height);/rgb_to_rgba_opaque(src_pixels, converted_pixels, (size_t)width \* (size_t)height);/' gles_texture.c
sed -i 's/const GLubyte \*src_pixels = (const GLubyte \*)pixels + y \* src_pitch;/const GLubyte \*src_pixels = (const GLubyte \*)pixels + (size_t)y \* src_pitch;/' gles_texture.c

# Fix the README.md to remove glCopyTexImage2D and glCopyTexSubImage2D from the TODO
sed -i 's/.*glCopyTexImage2D.*//' README.md
sed -i 's/.*glCopyTexSubImage2D.*//' README.md

