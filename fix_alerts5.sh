#!/bin/bash
git restore gles_texture.c
# Let's use more targeted Python script instead of sed to not mess up variable names like "data_width"

python3 -c "
import re
with open('gles_texture.c', 'r') as f:
    c = f.read()

# Fix the integer multiplications reported by CodeQL
c = c.replace('width * bytes_per_pixel', '(size_t)width * (size_t)bytes_per_pixel')
c = c.replace('width * height', '(size_t)width * (size_t)height')
c = c.replace('src_pitch * height', 'src_pitch * (size_t)height')
c = c.replace('tex_linear_size = (size_t)width * (size_t)height * (size_t)bytes_per_pixel', 'tex_linear_size = (size_t)width * (size_t)height * (size_t)bytes_per_pixel')
c = c.replace('glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h)', 'glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei width, GLsizei height)')
c = c.replace('void *pixels = GLI_MALLOC(w * h * 4);', 'void *pixels = GLI_MALLOC((size_t)width * (size_t)height * 4);')
c = c.replace('glReadPixels(x, y, w, h, format, type, pixels);', 'glReadPixels(x, y, width, height, format, type, pixels);')
c = c.replace('glTexSubImage2D(target, level, xoff, yoff, w, h, format, type, pixels);', 'glTexSubImage2D(target, level, xoff, yoff, width, height, format, type, pixels);')

# Now fix the row thing again since we did git restore
c = c.replace('for (GLsizei y = 0; y < height; y++) {', 'for (GLsizei row = 0; row < height; row++) {')
c = c.replace('memcpy(dst_pixels + y * xgu_texture->pitch, src_pixels + y * src_pitch, (size_t)width * (size_t)bytes_per_pixel);', 'memcpy(dst_pixels + row * xgu_texture->pitch, src_pixels + row * src_pitch, (size_t)width * (size_t)bytes_per_pixel);')
c = c.replace('src_pixels + y * src_pitch', 'src_pixels + row * src_pitch')
c = c.replace('yoff + y', 'yoff + row')

with open('gles_texture.c', 'w') as f:
    f.write(c)
"
