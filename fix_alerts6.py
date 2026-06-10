import re

with open('gles_texture.c', 'r') as f:
    c = f.read()

# Fix specific multiplications to prevent overflow instead of blanket replacements
# 1. glCopyTexImage2D
c = re.sub(
    r'void \*pixels = GLI_MALLOC\(width \* height \* bytes_per_pixel\);',
    r'void *pixels = GLI_MALLOC((size_t)width * (size_t)height * (size_t)bytes_per_pixel);',
    c
)

# 2. glCopyTexSubImage2D
c = re.sub(
    r'glCopyTexSubImage2D\(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h\)',
    r'glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei width, GLsizei height)',
    c
)
c = re.sub(
    r'void \*pixels = GLI_MALLOC\(w \* h \* 4\);',
    r'void *pixels = GLI_MALLOC((size_t)width * (size_t)height * 4);',
    c
)
c = re.sub(
    r'glReadPixels\(x, y, w, h, format, type, pixels\);',
    r'glReadPixels(x, y, width, height, format, type, pixels);',
    c
)
c = re.sub(
    r'glTexSubImage2D\(target, level, xoff, yoff, w, h, format, type, pixels\);',
    r'glTexSubImage2D(target, level, xoff, yoff, width, height, format, type, pixels);',
    c
)

# 3. CodeQL fixes
c = re.sub(
    r'rgb_to_rgba_opaque\(src_pixels, converted_pixels, width \* height\);',
    r'rgb_to_rgba_opaque(src_pixels, converted_pixels, (size_t)width * (size_t)height);',
    c
)
c = re.sub(
    r'const GLubyte \*src_pixels = \(const GLubyte \*\)pixels \+ y \* src_pitch;',
    r'const GLubyte *src_pixels = (const GLubyte *)pixels + (size_t)y * src_pitch;',
    c
)
c = re.sub(
    r'xgu_texture->pitch = xgu_texture->data_width \* bytes_per_pixel;',
    r'xgu_texture->pitch = (GLuint)xgu_texture->data_width * bytes_per_pixel;',
    c
)

# 4. Ambiguous y fixes
# This is tricky because we only want to change the inner loops
c = re.sub(
    r'for \(GLsizei y = 0; y < height; y\+\+\) \{(\s*)memcpy\(dst_pixels \+ y \* xgu_texture->pitch, src_pixels \+ y \* src_pitch, width \* bytes_per_pixel\);',
    r'for (GLsizei row = 0; row < height; row++) {\1memcpy(dst_pixels + row * xgu_texture->pitch, src_pixels + row * src_pitch, (size_t)width * (size_t)bytes_per_pixel);',
    c
)
c = re.sub(
    r'for \(GLsizei y = 0; y < height; y\+\+\) \{(\s*)memcpy\(linear_buf \+ \(yoff \+ y\) \* xgu_texture->pitch \+ xoff \* bytes_per_pixel,(\s*)src_pixels \+ y \* src_pitch,(\s*)width \* bytes_per_pixel\);',
    r'for (GLsizei row = 0; row < height; row++) {\1memcpy(linear_buf + (yoff + row) * xgu_texture->pitch + xoff * bytes_per_pixel,\2src_pixels + row * src_pitch,\3(size_t)width * (size_t)bytes_per_pixel);',
    c
)
c = re.sub(
    r'for \(GLsizei y = 0; y < height; y\+\+\) \{(\s*)memcpy\(dst_pixels \+ \(yoff \+ y\) \* xgu_texture->pitch \+ xoff \* bytes_per_pixel,(\s*)src_pixels \+ y \* src_pitch,(\s*)width \* bytes_per_pixel\);',
    r'for (GLsizei row = 0; row < height; row++) {\1memcpy(dst_pixels + (yoff + row) * xgu_texture->pitch + xoff * bytes_per_pixel,\2src_pixels + row * src_pitch,\3(size_t)width * (size_t)bytes_per_pixel);',
    c
)

with open('gles_texture.c', 'w') as f:
    f.write(c)

