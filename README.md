# OpenGL ES 1.1 implementation for nxdk
This is a mostly complete implementation of OpenGL ES 1.1 that works with [nxdk](https://github.com/XboxDev/nxdk.git). Screenshot shows Neverball rendering with nxdk-gless11 on real hardware!

![Screenshot1](/.github/image.png?)

## Features
### Core
* [x] All ES1.1 primitives
* [x] Fog
* [x] Lighting
* [x] Textures (Including NPOT)
* [x] Mipmaps
* [x] Vertex Buffer Objects (VBOs)
* [x] Clip Planes
* [x] Alpha/Depth/Stencil Functions
* [x] Automatic client-side array staging (malloc/stack memory)

### Extensions
* [x] Frame Buffer Objects (FBOs) (`GL_OES_framebuffer_object`)
* [x] Point Sprites (`GL_OES_point_sprite`)
* [x] Subtractive Blending (`GL_OES_blend_subtract`, `GL_OES_blend_equation_separate`)
* [x] 32-bit Indices (`GL_OES_element_index_uint`)
* [x] Point Size Arrays (`GL_OES_point_size_array`)

## How to use
### CMake
```
add_subdirectory(path/to/nxdk-gles11)
target_link_libraries(myapp PRIVATE GLESv1_CM)
target_link_libraries(myapp PRIVATE ${NXDK_DIR}/lib/libpbkit.lib)
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
    glSwapInterval(0); // Disable vsync. Default is 1 (Waits 1 vsync)

    // nxdk-gles11 automatically stages client-side arrays (like stack/malloc memory) 
    // into GPU-accessible memory, so you can pass them directly to gl*Pointer.
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

## Client-Side Array Staging
The NV2A GPU requires vertex data to be in physically contiguous memory to read it via DMA. nxdk-gles11 includes a staging arena that automatically handles copying client-side arrays (e.g., from `malloc` or the stack) into contiguous, GPU-accessible memory before drawing.

* **VBOs:** If you use Vertex Buffer Objects (`glGenBuffers`, `glBindBuffer`), the data is already stored in contiguous memory and staging is bypassed for maximum performance.
* **Arena Size:** The staging arena defaults to 2MB. If you have very large client-side draws and see an out-of-memory error in the debug output, you can increase this by defining `GLI_STAGING_ARENA_SIZE` before building.


## Todo
* [ ] Lots of FIXMEs
* [ ] glTexSubImage2D, glCopyTexImage2D, glCopyTexSubImage2D (and compressed?)
* [ ] Replace swizzle code with something more permissive (MIT etc)

## Attribution
* Some lighting code taken from https://github.com/JayFoxRox/xgu-gl
* nv2a help from https://github.com/abaire/nxdk_pgraph_tests
* xgu wrapper for pbkit https://github.com/dracc/xgu.git (MIT)
* cglm matrix code https://github.com/recp/cglm (MIT)
* Swizzle Code https://github.com/xemu-project/xemu/blob/9917817a8ac0c3d9826d9d005e76379ef1eb4b38/hw/xbox/nv2a/pgraph/swizzle.c (GPLv2)
