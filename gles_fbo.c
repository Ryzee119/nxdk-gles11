#include "gles_private.h"
#include <swizzle.h>

static XguTexFormatColor unswizzle_texture_format(XguTexFormatColor format)
{
    switch (format) {
        case XGU_TEXTURE_FORMAT_A8_SWIZZLED:
            return XGU_TEXTURE_FORMAT_A8;
        case XGU_TEXTURE_FORMAT_Y8_SWIZZLED:
            return XGU_TEXTURE_FORMAT_Y8;
        case XGU_TEXTURE_FORMAT_A8Y8_SWIZZLED:
            return XGU_TEXTURE_FORMAT_A8Y8;
        case XGU_TEXTURE_FORMAT_A8R8G8B8_SWIZZLED:
            return XGU_TEXTURE_FORMAT_A8R8G8B8;
        case XGU_TEXTURE_FORMAT_R5G6B5_SWIZZLED:
            return XGU_TEXTURE_FORMAT_R5G6B5;
        case XGU_TEXTURE_FORMAT_A4R4G4B4_SWIZZLED:
            return XGU_TEXTURE_FORMAT_A4R4G4B4;
        case XGU_TEXTURE_FORMAT_A1R5G5B5_SWIZZLED:
            return XGU_TEXTURE_FORMAT_A1R5G5B5;
        default:
            return format;
    }
}

framebuffer_object_t *gliFindFramebufferObject(GLuint name, framebuffer_object_t **prev)
{
    gli_context_t *context = gliGetContext();
    if (prev) {
        *prev = NULL;
    }
    if (name == 0) {
        return NULL;
    }
    for (framebuffer_object_t *it = context->framebuffer_objects; it != NULL; it = it->next) {
        if (it->name == name) {
            return it;
        }
        if (prev) {
            *prev = it;
        }
    }
    return NULL;
}

renderbuffer_object_t *gliFindRenderbufferObject(GLuint name, renderbuffer_object_t **prev)
{
    gli_context_t *context = gliGetContext();
    if (prev) {
        *prev = NULL;
    }
    if (name == 0) {
        return NULL;
    }
    for (renderbuffer_object_t *it = context->renderbuffer_objects; it != NULL; it = it->next) {
        if (it->name == name) {
            return it;
        }
        if (prev) {
            *prev = it;
        }
    }
    return NULL;
}

GL_API void GL_APIENTRY glGenFramebuffersOES(GLsizei n, GLuint *framebuffers)
{
    if (framebuffers == NULL) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if (n < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; i++) {
        // Surely we will never wrap around, but skip zero if we so
        // Fixme, wrap around could cause duplicate names
        if (next == 0) {
            next++;
        }
        framebuffers[i] = next++;
    }
}

GL_API void GL_APIENTRY glBindFramebufferOES(GLenum target, GLuint framebuffer)
{
    gli_context_t *context = gliGetContext();
    if (!context) {
        return;
    }

    if (target != GL_FRAMEBUFFER_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    // If the framebuffer name is zero just unbind any texture currently bound to the target
    if (framebuffer == 0) {
        if (context->fbo_binding != 0) {
            context->fbo_binding = 0;
            context->fbo_state_dirty = GL_TRUE;
            context->transformation_state.viewport_dirty = GL_TRUE;
            gliFBOFlush();
        }
        return;
    }

    // First check if the object already exists, if so just bind it to the texture_unit and we are done
    framebuffer_object_t *fbo = gliFindFramebufferObject(framebuffer, NULL);
    if (fbo != NULL) {
        if (context->fbo_binding != framebuffer) {
            context->fbo_binding = framebuffer;
            context->fbo_state_dirty = GL_TRUE;
            context->transformation_state.viewport_dirty = GL_TRUE;
            gliFBOFlush();
        }
        return;
    }

    // It's a new object, so we create it
    fbo = GLI_MALLOC(sizeof(framebuffer_object_t));
    if (fbo == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    // The state of a framebuffer object immediately after it is first bound is three attachment points
    // (GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, and GL_STENCIL_ATTACHMENT) each with GL_NONE as the object type.
    gli_memset(fbo, 0, sizeof(framebuffer_object_t));
    fbo->color.type = GL_NONE_OES;
    fbo->depth.type = GL_NONE_OES;
    fbo->stencil.type = GL_NONE_OES;
    fbo->name = framebuffer;

    // Bind the fbo to the context
    context->fbo_binding = framebuffer;
    context->fbo_state_dirty = GL_TRUE;

    // Add it to the context list
    fbo->next = context->framebuffer_objects;
    context->framebuffer_objects = fbo;
    context->transformation_state.viewport_dirty = GL_TRUE;

    gliFBOFlush();
}

GL_API void GL_APIENTRY glDeleteFramebuffersOES(GLsizei n, const GLuint *framebuffers)
{
    gli_context_t *context = gliGetContext();
    if (!framebuffers) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (n < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    for (GLint i = 0; i < n; i++) {
        GLuint name = framebuffers[i];
        framebuffer_object_t *prev = NULL;
        framebuffer_object_t *fbo = gliFindFramebufferObject(name, &prev);
        if (fbo) {
            // Remove from the context's list
            if (prev) {
                prev->next = fbo->next;
            } else {
                context->framebuffer_objects = fbo->next;
            }

            // If a fbo that is currently bound is deleted, the binding reverts to 0
            if (context->fbo_binding == name) {
                context->fbo_binding = 0;
                context->fbo_state_dirty = GL_TRUE;
            }
            GLI_FREE(fbo);
        }
    }
}

GL_API GLboolean GL_APIENTRY glIsFramebufferOES(GLuint framebuffer)
{
    gli_context_t *context = gliGetContext();
    framebuffer_object_t *fbo = gliFindFramebufferObject(framebuffer, NULL);
    if (framebuffer == 0 || fbo == NULL) {
        return GL_FALSE;
    }
    return GL_TRUE;
}

GL_API void GL_APIENTRY glGenRenderbuffersOES(GLsizei n, GLuint *renderbuffers)
{
    if (renderbuffers == NULL) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if (n < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; i++) {
        // Surely we will never wrap around, but skip zero if we so
        // Fixme, wrap around could cause duplicate names
        if (next == 0) {
            next++;
        }
        renderbuffers[i] = next++;
    }
}

GL_API void GL_APIENTRY glBindRenderbufferOES(GLenum target, GLuint renderbuffer)
{
    gli_context_t *context = gliGetContext();
    if (!context) {
        return;
    }

    if (target != GL_RENDERBUFFER_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    // If the renderbuffer name is zero just unbind from the context
    if (renderbuffer == 0) {
        context->rbo_binding = 0;
    }

    // First check if the object already exists, if so just bind it to the texture_unit and we are done
    renderbuffer_object_t *rbo = gliFindRenderbufferObject(renderbuffer, NULL);
    if (rbo != NULL) {
        if (context->rbo_binding != renderbuffer) {
            context->rbo_binding = renderbuffer;
        }
        return;
    }

    // It's a new object, so we create it
    rbo = GLI_MALLOC(sizeof(renderbuffer_object_t));
    if (rbo == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    // The state of a framebuffer object immediately after it is first bound is three attachment points
    // (GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, and GL_STENCIL_ATTACHMENT) each with GL_NONE as the object type.
    gli_memset(rbo, 0, sizeof(renderbuffer_object_t));
    rbo->name = renderbuffer;

    // Bind the fbo to the context
    context->rbo_binding = renderbuffer;

    // Add it to the context list
    rbo->next = context->renderbuffer_objects;
    context->renderbuffer_objects = rbo;
}

GL_API void GL_APIENTRY glDeleteRenderbuffersOES(GLsizei n, const GLuint *renderbuffers)
{
    gli_context_t *context = gliGetContext();
    if (!renderbuffers) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (n < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    for (GLint i = 0; i < n; i++) {
        GLuint name = renderbuffers[i];
        renderbuffer_object_t *prev = NULL;
        renderbuffer_object_t *rbo = gliFindRenderbufferObject(name, &prev);
        if (rbo) {
            // Remove from the context's list
            if (prev) {
                prev->next = rbo->next;
            } else {
                context->renderbuffer_objects = rbo->next;
            }

            // If a fbo that is currently bound is deleted, the binding reverts to 0
            if (context->rbo_binding == name) {
                context->rbo_binding = 0;
            }

            if (rbo->data) {
                MmFreeContiguousMemory(rbo->data);
            }

            // When deleting an RBO, iterate through all FBOs in context->framebuffer_objects.
            // If any attachment references the deleted RBO name, reset that attachment's type to GL_NONE_OES and clear
            // its pointer.
            for (framebuffer_object_t *it = context->framebuffer_objects; it != NULL; it = it->next) {
                if (it->color.name == rbo->name) {
                    it->color.type = GL_NONE_OES;
                }
                if (it->depth.name == rbo->name) {
                    it->depth.type = GL_NONE_OES;
                }
                if (it->stencil.name == rbo->name) {
                    it->stencil.type = GL_NONE_OES;
                }
            }
            GLI_FREE(rbo);
        }
    }
}

GL_API GLboolean GL_APIENTRY glIsRenderbufferOES(GLuint renderbuffer)
{
    gli_context_t *context = gliGetContext();
    renderbuffer_object_t *rbo = gliFindRenderbufferObject(renderbuffer, NULL);
    if (renderbuffer == 0 || rbo == NULL) {
        return GL_FALSE;
    }
    return GL_TRUE;
}

GL_API void GL_APIENTRY glRenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    gli_context_t *context = gliGetContext();
    if (target != GL_RENDERBUFFER_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (context->rbo_binding == 0) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }
    renderbuffer_object_t *rbo = gliFindRenderbufferObject(context->rbo_binding, NULL);
    if (!rbo) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    switch (internalformat) {
        case GL_DEPTH_COMPONENT16_OES:
        case GL_DEPTH_COMPONENT24_OES:
        case GL_STENCIL_INDEX8_OES:
        case GL_DEPTH24_STENCIL8_OES:
            // All depth/stencil formats are promoted to 24-bit depth with packed 8 bit stencil for xbox
            internalformat = GL_DEPTH24_STENCIL8_OES;
            break;
        case GL_RGBA8_OES:
        case GL_RGB8_OES:
        case GL_RGB565_OES:
        case GL_RGB5_A1_OES:
        case GL_RGBA4_OES:
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
    uint32_t bpp = gliFormatToBpp(internalformat);

    if (rbo->data) {
        MmFreeContiguousMemory(rbo->data);
        rbo->data = NULL;
        rbo->data_physical_address = NULL;
    }

    rbo->swizzled = GL_FALSE;

    const uint32_t pitch = (width * bpp + 63) & ~63;

    uint32_t size = pitch * height;
    rbo->data = MmAllocateContiguousMemoryEx(size, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (!rbo->data) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }
    rbo->data_physical_address = (void *)MmGetPhysicalAddress(rbo->data);
    gli_memset(rbo->data, 0, size);
    rbo->data_physical_address = (GLubyte *)MmGetPhysicalAddress(rbo->data);
    rbo->width = width;
    rbo->height = height;
    rbo->internalformat = internalformat;
    context->fbo_state_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glFramebufferRenderbufferOES(GLenum target,
                                                     GLenum attachment,
                                                     GLenum renderbuffertarget,
                                                     GLuint renderbuffer)
{
    if (target != GL_FRAMEBUFFER_OES || (renderbuffertarget != GL_RENDERBUFFER_OES && renderbuffer != 0)) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    gli_context_t *context = gliGetContext();
    if (context->fbo_binding == 0) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }
    framebuffer_object_t *fbo = gliFindFramebufferObject(context->fbo_binding, NULL);
    if (!fbo) {
        return;
    }

    framebuffer_attachment_t *att = NULL;
    if (attachment == GL_COLOR_ATTACHMENT0_OES) {
        att = &fbo->color;
    } else if (attachment == GL_DEPTH_ATTACHMENT_OES) {
        att = &fbo->depth;
    } else if (attachment == GL_STENCIL_ATTACHMENT_OES) {
        att = &fbo->stencil;
    } else {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (renderbuffer == 0) {
        att->type = GL_NONE_OES;
        att->name = 0;
        att->renderbuffer = NULL;
    } else {
        renderbuffer_object_t *rbo = gliFindRenderbufferObject(renderbuffer, NULL);
        if (!rbo) {
            gliSetError(GL_INVALID_OPERATION);
            return;
        }
        att->type = GL_RENDERBUFFER_OES;
        att->name = renderbuffer;
        att->renderbuffer = rbo;
    }
    context->fbo_state_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY
glFramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    if (target != GL_FRAMEBUFFER_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (level != 0) { // mipmaps not fully supported as render targets here?
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    gli_context_t *context = gliGetContext();
    if (context->fbo_binding == 0) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }
    framebuffer_object_t *fbo = gliFindFramebufferObject(context->fbo_binding, NULL);
    if (!fbo) {
        return;
    }

    framebuffer_attachment_t *att = NULL;
    if (attachment == GL_COLOR_ATTACHMENT0_OES) {
        att = &fbo->color;
    } else if (attachment == GL_DEPTH_ATTACHMENT_OES) {
        att = &fbo->depth;
    } else if (attachment == GL_STENCIL_ATTACHMENT_OES) {
        att = &fbo->stencil;
    } else {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (texture == 0) {
        att->type = GL_NONE_OES;
        att->name = 0;
        att->texture = NULL;
    } else {
        texture_object_t *tex = gliFindTextureObject(texture, NULL);
        if (!tex || tex->texture_2d == NULL) {
            gliSetError(GL_INVALID_OPERATION);
            return;
        }

        xgu_texture_t *xtex = (xgu_texture_t *)tex->texture_2d;

        // Convert swizzled textures to linear when attached to an FBO.
        // This texture will stay linear forever now. FIXME?
        if (xtex && xtex->swizzled) {
            uint32_t new_pitch = (xtex->data_width * xtex->bytes_per_pixel + 63) & ~63;
            uint32_t size = new_pitch * xtex->data_height;

            void *new_data =
                MmAllocateContiguousMemoryEx(size, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
            if (new_data) {
                unswizzle_rect(
                    xtex->data, xtex->data_width, xtex->data_height, new_data, new_pitch, xtex->bytes_per_pixel);
                MmFreeContiguousMemory(xtex->data);

                xtex->data = new_data;
                xtex->data_physical_address = (void *)MmGetPhysicalAddress(new_data);
                xtex->pitch = new_pitch;
                xtex->swizzled = 0;
                xtex->u_scale = (GLfloat)xtex->data_width;
                xtex->v_scale = (GLfloat)xtex->data_height;
                xtex->format = unswizzle_texture_format(xtex->format);
            } else {
                gliSetError(GL_OUT_OF_MEMORY);
                return;
            }
        }

        att->type = GL_TEXTURE_2D;
        att->name = texture;
        att->texture = tex;
        att->level = level;
    }
    context->fbo_state_dirty = GL_TRUE;
}

static void get_attachment_dims(framebuffer_attachment_t *att, int *w, int *h)
{
    *w = 0;
    *h = 0;
    if (att->type == GL_RENDERBUFFER_OES && att->renderbuffer) {
        *w = att->renderbuffer->width;
        *h = att->renderbuffer->height;
    } else if (att->type == GL_TEXTURE_2D && att->texture && att->texture->texture_2d) {
        xgu_texture_t *xtex = (xgu_texture_t *)att->texture->texture_2d;
        *w = xtex->tex_width;
        *h = xtex->tex_height;
    }
}

GL_API GLenum GL_APIENTRY glCheckFramebufferStatusOES(GLenum target)
{
    if (target != GL_FRAMEBUFFER_OES) {
        gliSetError(GL_INVALID_ENUM);
        return 0;
    }
    gli_context_t *context = gliGetContext();
    if (context->fbo_binding == 0) {
        return GL_FRAMEBUFFER_COMPLETE_OES;
    }

    framebuffer_object_t *fbo = gliFindFramebufferObject(context->fbo_binding, NULL);
    if (!fbo) {
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
    }

    if (fbo->color.type == GL_NONE_OES && fbo->depth.type == GL_NONE_OES && fbo->stencil.type == GL_NONE_OES) {
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES;
    }

    int width = -1;
    int height = -1;

#define VALIDATE_ATTACHMENT(att)                                                                                       \
    do {                                                                                                               \
        if ((att).type != GL_NONE_OES) {                                                                               \
            int w, h;                                                                                                  \
            get_attachment_dims(&(att), &w, &h);                                                                       \
            if (w <= 0 || h <= 0)                                                                                      \
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES;                                                       \
            if (width == -1) {                                                                                         \
                width = w;                                                                                             \
                height = h;                                                                                            \
            } else if (width != w || height != h) {                                                                    \
                return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES;                                                       \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

    VALIDATE_ATTACHMENT(fbo->color);
    VALIDATE_ATTACHMENT(fbo->depth);
    VALIDATE_ATTACHMENT(fbo->stencil);

#undef VALIDATE_ATTACHMENT
    return GL_FRAMEBUFFER_COMPLETE_OES;
}

GL_API void GL_APIENTRY glGetFramebufferAttachmentParameterivOES(GLenum target,
                                                                 GLenum attachment,
                                                                 GLenum pname,
                                                                 GLint *params)
{
    if (target != GL_FRAMEBUFFER_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    gli_context_t *context = gliGetContext();
    if (context->fbo_binding == 0) {
        // Can't query default framebuffer
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    framebuffer_object_t *fbo = gliFindFramebufferObject(context->fbo_binding, NULL);
    if (!fbo) {
        return;
    }

    framebuffer_attachment_t *att = NULL;
    if (attachment == GL_COLOR_ATTACHMENT0_OES) {
        att = &fbo->color;
    } else if (attachment == GL_DEPTH_ATTACHMENT_OES) {
        att = &fbo->depth;
    } else if (attachment == GL_STENCIL_ATTACHMENT_OES) {
        att = &fbo->stencil;
    } else {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES) {
        *params = att->type;
    } else if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES) {
        *params = att->name;
    } else if (pname == GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_OES && att->type == GL_TEXTURE_2D) {
        *params = att->level;
    } else {
        gliSetError(GL_INVALID_ENUM);
    }
}

GL_API void GL_APIENTRY glGetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint *params)
{
    if (target != GL_RENDERBUFFER_OES) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    gli_context_t *context = gliGetContext();
    if (context->rbo_binding == 0) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }
    renderbuffer_object_t *rbo = gliFindRenderbufferObject(context->rbo_binding, NULL);
    if (!rbo) {
        return;
    }

    switch (pname) {
        case GL_RENDERBUFFER_WIDTH_OES:
            *params = rbo->width;
            break;
        case GL_RENDERBUFFER_HEIGHT_OES:
            *params = rbo->height;
            break;
        case GL_RENDERBUFFER_INTERNAL_FORMAT_OES:
            *params = rbo->internalformat;
            break;
        case GL_RENDERBUFFER_RED_SIZE_OES:
            if (rbo->internalformat == GL_RGBA8_OES || rbo->internalformat == GL_RGB8_OES) {
                *params = 8;
            } else if (rbo->internalformat == GL_RGB565_OES || rbo->internalformat == GL_RGB5_A1_OES) {
                *params = 5;
            } else if (rbo->internalformat == GL_RGBA4_OES) {
                *params = 4;
            } else {
                *params = 0;
            }
            break;
        case GL_RENDERBUFFER_GREEN_SIZE_OES:
            if (rbo->internalformat == GL_RGBA8_OES || rbo->internalformat == GL_RGB8_OES) {
                *params = 8;
            } else if (rbo->internalformat == GL_RGB565_OES) {
                *params = 6;
            } else if (rbo->internalformat == GL_RGB5_A1_OES) {
                *params = 5;
            } else if (rbo->internalformat == GL_RGBA4_OES) {
                *params = 4;
            } else {
                *params = 0;
            }
            break;
        case GL_RENDERBUFFER_BLUE_SIZE_OES:
            if (rbo->internalformat == GL_RGBA8_OES || rbo->internalformat == GL_RGB8_OES) {
                *params = 8;
            } else if (rbo->internalformat == GL_RGB565_OES || rbo->internalformat == GL_RGB5_A1_OES) {
                *params = 5;
            } else if (rbo->internalformat == GL_RGBA4_OES) {
                *params = 4;
            } else {
                *params = 0;
            }
            break;
        case GL_RENDERBUFFER_ALPHA_SIZE_OES:
            if (rbo->internalformat == GL_RGBA8_OES) {
                *params = 8;
            } else if (rbo->internalformat == GL_RGB5_A1_OES) {
                *params = 1;
            } else if (rbo->internalformat == GL_RGBA4_OES) {
                *params = 4;
            } else {
                *params = 0;
            }
            break;
        case GL_RENDERBUFFER_DEPTH_SIZE_OES:
            if (rbo->internalformat == GL_DEPTH_COMPONENT16_OES) {
                *params = 16;
            } else if (rbo->internalformat == GL_DEPTH_COMPONENT24_OES ||
                       rbo->internalformat == GL_DEPTH24_STENCIL8_OES) {
                *params = 24;
            } else {
                *params = 0;
            }
            break;
        case GL_RENDERBUFFER_STENCIL_SIZE_OES:
            if (rbo->internalformat == GL_STENCIL_INDEX8_OES || rbo->internalformat == GL_DEPTH24_STENCIL8_OES) {
                *params = 8;
            } else {
                *params = 0;
            }
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            break;
    }
}

void gliFBOFlush(void)
{
    gli_context_t *context = gliGetContext();
    if (!context || !context->fbo_state_dirty) {
        return;
    }

    context->fbo_state_dirty = GL_FALSE;

    // When leaving an FBO back to the default buffer, restore pbkit state cleanly
    if (context->fbo_binding == 0) {
        pb_target_back_buffer();
        context->current_surface_format =
            XGU_MASK(NV097_SET_SURFACE_FORMAT_COLOR, NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8) |
            XGU_MASK(NV097_SET_SURFACE_FORMAT_ZETA, NV097_SET_SURFACE_FORMAT_ZETA_Z24S8) |
            XGU_MASK(NV097_SET_SURFACE_FORMAT_TYPE, NV097_SET_SURFACE_FORMAT_TYPE_PITCH);
        context->current_surface_width = pb_back_buffer_width();
        context->current_surface_height = pb_back_buffer_height();
        uint32_t *p = pb_begin();
        p = xgu_set_front_face(p,
                               (context->rasterization_state.cull_front_face == GL_CCW) ? XGU_FRONT_CCW : XGU_FRONT_CW);
        pb_end(p);
        return;
    }

    framebuffer_object_t *fbo = gliFindFramebufferObject(context->fbo_binding, NULL);
    if (!fbo || glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
        return;
    }

    void *color_data = NULL;
    void *depth_data = NULL;
    uint32_t pitch;
    uint32_t zpitch;
    uint32_t clip_width;
    uint32_t clip_height;

    extern unsigned int pb_ColorFmt;
    uint32_t fmt_color = pb_ColorFmt;

    // Currently all depth/stencil buffers are forced for Z24S8 and surfaces are pitched
    // Swizzle surfaces are not currently supported. GlClear on swizzled surfaces is just messy
    uint32_t fmt_zeta = NV097_SET_SURFACE_FORMAT_ZETA_Z24S8;
    uint32_t fmt_type = NV097_SET_SURFACE_FORMAT_TYPE_PITCH;

    // Resolve Color Attachment
    if (fbo->color.type == GL_TEXTURE_2D && fbo->color.texture && fbo->color.texture->texture_2d) {
        xgu_texture_t *tex = (xgu_texture_t *)fbo->color.texture->texture_2d;
        color_data = tex->data;
        clip_width = tex->tex_width;
        clip_height = tex->tex_height;
        pitch = (tex->pitch + 63) & ~63;

        switch (tex->format) {
            case XGU_TEXTURE_FORMAT_R5G6B5_SWIZZLED:
            case XGU_TEXTURE_FORMAT_R5G6B5:
                fmt_color = NV097_SET_SURFACE_FORMAT_COLOR_LE_R5G6B5;
                break;
            case XGU_TEXTURE_FORMAT_A1R5G5B5_SWIZZLED:
            case XGU_TEXTURE_FORMAT_A1R5G5B5:
                fmt_color = NV097_SET_SURFACE_FORMAT_COLOR_LE_X1R5G5B5_O1R5G5B5;
                break;
            case XGU_TEXTURE_FORMAT_X8R8G8B8_SWIZZLED:
            case XGU_TEXTURE_FORMAT_X8R8G8B8:
                fmt_color = NV097_SET_SURFACE_FORMAT_COLOR_LE_X8R8G8B8_Z8R8G8B8;
                break;
            case XGU_TEXTURE_FORMAT_A8R8G8B8_SWIZZLED:
            case XGU_TEXTURE_FORMAT_A8R8G8B8:
                fmt_color = NV097_SET_SURFACE_FORMAT_COLOR_LE_A8R8G8B8;
                break;
            default:
                assert(0);
                return;
        }

    } else if (fbo->color.type == GL_RENDERBUFFER_OES && fbo->color.renderbuffer) {
        renderbuffer_object_t *rbo = fbo->color.renderbuffer;
        color_data = rbo->data;
        clip_width = rbo->width;
        clip_height = rbo->height;

        int bpp = gliFormatToBpp(rbo->internalformat);
        fmt_color = gliFormatToNvSurfaceFormat(rbo->internalformat);
        pitch = (rbo->width * bpp + 63) & ~63;

    } else {
        pitch = pb_back_buffer_pitch();
        clip_width = pb_back_buffer_width();
        clip_height = pb_back_buffer_height();
    }

    // Resolve Depth / Stencil Attachment
    renderbuffer_object_t *zeta_rbo = fbo->depth.renderbuffer ? fbo->depth.renderbuffer : fbo->stencil.renderbuffer;
    if (zeta_rbo) {
        depth_data = zeta_rbo->data;
        zpitch = (zeta_rbo->width * 4 + 63) & ~63;
    } else {
        zpitch = pb_back_buffer_pitch();
    }

    const uint32_t log2_width = __builtin_ctz(clip_width);
    const uint32_t log2_height = __builtin_ctz(clip_height);
    const uint32_t format =
        XGU_MASK(NV097_SET_SURFACE_FORMAT_COLOR, fmt_color) | XGU_MASK(NV097_SET_SURFACE_FORMAT_ZETA, fmt_zeta) |
        XGU_MASK(NV097_SET_SURFACE_FORMAT_TYPE, fmt_type) | XGU_MASK(NV097_SET_SURFACE_FORMAT_WIDTH, log2_width) |
        XGU_MASK(NV097_SET_SURFACE_FORMAT_HEIGHT, log2_height);

    static struct s_CtxDma dma_color;
    static struct s_CtxDma dma_depth;
    static bool dma_init = false;
    if (!dma_init) {
        pb_create_dma_ctx(20, DMA_CLASS_3D, 0, MAXRAM, &dma_color);
        pb_create_dma_ctx(21, DMA_CLASS_3D, 0, MAXRAM, &dma_depth);
        pb_bind_channel(&dma_color);
        pb_bind_channel(&dma_depth);
        dma_init = true;
    }

    uint32_t ctx_color = 9; // DMA_CHANNEL_PIXEL_RENDERER
    if (color_data) {
        pb_set_dma_address(&dma_color, color_data, 0xFFFFFFFF);
        ctx_color = 20;
    }

    uint32_t ctx_zeta = 10; // DMA_CHANNEL_DEPTH_STENCIL_RENDERER
    if (depth_data) {
        pb_set_dma_address(&dma_depth, depth_data, 0xFFFFFFFF);
        ctx_zeta = 21;
    }

    uint32_t *p;
    p = pb_begin();
    p = pb_push1(p, NV097_WAIT_FOR_IDLE, 0);
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_COLOR, ctx_color);
    p = pb_push1(p, NV097_SET_CONTEXT_DMA_ZETA, ctx_zeta);
    p = pb_push1(p,
                 NV097_SET_SURFACE_PITCH,
                 XGU_MASK(NV097_SET_SURFACE_PITCH_COLOR, pitch) | XGU_MASK(NV097_SET_SURFACE_PITCH_ZETA, zpitch));
    p = pb_push1(p, NV097_SET_SURFACE_COLOR_OFFSET, 0);
    p = pb_push1(p, NV097_SET_SURFACE_ZETA_OFFSET, 0);
    p = pb_push1(p,
                 NV097_SET_SURFACE_CLIP_HORIZONTAL,
                 XGU_MASK(NV097_SET_SURFACE_CLIP_HORIZONTAL_WIDTH, clip_width) |
                     XGU_MASK(NV097_SET_SURFACE_CLIP_HORIZONTAL_X, 0));
    p = pb_push1(p,
                 NV097_SET_SURFACE_CLIP_VERTICAL,
                 XGU_MASK(NV097_SET_SURFACE_CLIP_VERTICAL_HEIGHT, clip_height) |
                     XGU_MASK(NV097_SET_SURFACE_CLIP_VERTICAL_Y, 0));
    p = pb_push1(p, NV097_SET_SURFACE_FORMAT, format);
    p = pb_push1(p,
                 NV097_SET_FRONT_FACE,
                 (context->rasterization_state.cull_front_face == GL_CCW) ? NV097_SET_FRONT_FACE_V_CW
                                                                          : NV097_SET_FRONT_FACE_V_CCW);
    pb_end(p);

    context->current_surface_format = format;
    context->current_surface_width = clip_width;
    context->current_surface_height = clip_height;
}
