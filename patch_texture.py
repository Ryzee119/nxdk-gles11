import re

with open('gles_texture.c', 'r') as f:
    content = f.read()

copy_tex_image_pattern = re.compile(
    r'GL_API void GL_APIENTRY glCopyTexImage2D\(\s*GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border\)\s*\{[^}]*\}',
    re.MULTILINE | re.DOTALL
)

copy_tex_image_replacement = """GL_API void GL_APIENTRY glCopyTexImage2D(
    GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    GLenum format = internalformat;
    GLenum type = GL_UNSIGNED_BYTE;
    GLsizei bytes_per_pixel = 0;

    switch (internalformat) {
    case GL_ALPHA:
    case GL_LUMINANCE:
        bytes_per_pixel = 1;
        break;
    case GL_LUMINANCE_ALPHA:
        bytes_per_pixel = 2;
        break;
    case GL_RGB:
        bytes_per_pixel = 3;
        break;
    case GL_RGBA:
        bytes_per_pixel = 4;
        break;
    default:
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    void *pixels = GLI_MALLOC(width * height * bytes_per_pixel);
    if (!pixels) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    glReadPixels(x, y, width, height, format, type, pixels);
    glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    GLI_FREE(pixels);
}"""

content = copy_tex_image_pattern.sub(copy_tex_image_replacement, content)

copy_tex_sub_image_pattern = re.compile(
    r'GL_API void GL_APIENTRY\s*glCopyTexSubImage2D\(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h\)\s*\{[^}]*\}',
    re.MULTILINE | re.DOTALL
)

copy_tex_sub_image_replacement = """GL_API void GL_APIENTRY
glCopyTexSubImage2D(GLenum target, GLint level, GLint xoff, GLint yoff, GLint x, GLint y, GLsizei w, GLsizei h)
{
    GLenum format = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
    void *pixels = GLI_MALLOC(w * h * 4);
    if (!pixels) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    glReadPixels(x, y, w, h, format, type, pixels);
    glTexSubImage2D(target, level, xoff, yoff, w, h, format, type, pixels);
    GLI_FREE(pixels);
}"""

content = copy_tex_sub_image_pattern.sub(copy_tex_sub_image_replacement, content)

with open('gles_texture.c', 'w') as f:
    f.write(content)
