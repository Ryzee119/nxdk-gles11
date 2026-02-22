# OpenGL ES 1.1 implementation for nxdk
This is a mostly complete implementation of OpenGL ES 1.1 that works with [nxdk](https://github.com/XboxDev/nxdk.git). Screenshot shows Neverball rendering with nxdk-gless11 on real hardware!

![Screenshot1](/.github/image.png?)

## Features
* [x] All ES1.1 primitives
* [x] Fog
* [x] Lighting
* [x] Textures (Including NPOT)
* [x] Vertex Buffer Objects (VBOs)
* [x] Point Sprites
* [x] Clip Planes
* [x] Alpha/Depth/Stencil Functions

## How to use
### CMake
```
add_subdirectory(path/to/nxdk-gles11)
target_link_libraries(myapp PRIVATE GLESv1_CM)
```
### Makefile
Fixme

## Usage
```c
#include <GLES/gl.h>
#include <hal/video.h>

int main (int argc, char **argv)
{
    XVideoSetMode(640, 480, 32, REFRESH_DEFAULT);
    glContextInit(640, 480);

    // In real code you would want to allocate memory with MmAllocateContiguousMemory and use that for your vertex
    // data, but for this simple test we can just use the stack.
    GLfloat vertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f, 
         0.5f, -0.5f, 0.0f   
    };

    GLfloat colors[] = {
        1.0f, 0.0f, 0.0f, 1.0f,  
        0.0f, 1.0f, 0.0f, 1.0f,  
        0.0f, 0.0f, 1.0f, 1.0f   
    };

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, colors);

    while (1) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glFlipNV2A(); // Actually draws to screen (on vblank)
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    return 0;
}
```

## Todo
* [ ] Lots of FIXMEs
* [ ] Mipmaps
* [ ] Framebuffer Objects
* [ ] glTexSubImage2D, glCopyTexImage2D, glCopyTexSubImage2D (and compressed?)
* [ ] Replace swizzle code with something more permissive (MIT etc)

## Attribution
* Some lighting code taken from https://github.com/JayFoxRox/xgu-gl
* nv2a help from https://github.com/abaire/nxdk_pgraph_tests
* xgu wrapper for pbkit https://github.com/dracc/xgu.git (MIT)
* cglm matrix code https://github.com/recp/cglm (MIT)
* Swizzle Code https://github.com/xemu-project/xemu/blob/9917817a8ac0c3d9826d9d005e76379ef1eb4b38/hw/xbox/nv2a/pgraph/swizzle.c (GPLv2)