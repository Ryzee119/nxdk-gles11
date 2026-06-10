import re

with open('gles_texture.c', 'r') as f:
    content = f.read()

# We need to figure out the internal format of the currently bound texture
# to use the right format/type for glReadPixels when calling glTexSubImage2D,
# or we can just use GL_RGBA and it will work if glReadPixels supports it.
# The OpenGL ES 1.1 spec says for glCopyTexSubImage2D:
# "pixels are exactly as if glReadPixels had been called with format set to GL_RGBA, GL_RGB, GL_LUMINANCE_ALPHA, GL_LUMINANCE, or GL_ALPHA depending on the internal format of the currently bound texture."

# However, we don't know the internal format easily without querying it.
# But looking at glTexSubImage2D, it takes format and type of the pixel data we supply!
# So we can just use GL_RGBA and GL_UNSIGNED_BYTE as an intermediate format.
# It should be converted appropriately by glTexSubImage2D. Let's make sure our glTexSubImage2D supports GL_RGBA!
