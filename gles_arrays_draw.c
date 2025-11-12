// gles_arrays_draw.c
#include "gles_private.h"

static void glEnableDisableClientState(GLenum array, GLboolean enable)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;

    switch (array) {
        case GL_VERTEX_ARRAY:
            vad->vertex_array_enabled = enable;
            vad->vertex_array_dirty = GL_TRUE;
            break;
        case GL_NORMAL_ARRAY:
            vad->normal_array_enabled = enable;
            vad->normal_array_dirty = GL_TRUE;
            break;
        case GL_COLOR_ARRAY:
            vad->color_array_enabled = enable;
            vad->color_array_dirty = GL_TRUE;
            break;
        case GL_TEXTURE_COORD_ARRAY:
            const GLenum texture = vad->client_active_texture - GL_TEXTURE0;
            vad->texcoord_array_enabled[texture] = enable;
            vad->texcoord_array_dirty[texture] = GL_TRUE;
            break;
        case GL_POINT_SIZE_ARRAY_OES:
            vad->point_size_array_enabled = enable;
            vad->point_size_array_dirty = GL_TRUE;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            break;
    }
}

GL_API void GL_APIENTRY glEnableClientState(GLenum array)
{
    glEnableDisableClientState(array, GL_TRUE);
}

GL_API void GL_APIENTRY glDisableClientState(GLenum array)
{
    glEnableDisableClientState(array, GL_FALSE);
}

GL_API void GL_APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;

    if (stride < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_BYTE && type != GL_SHORT && type != GL_FIXED && type != GL_FLOAT) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (size < 2 || size > 4) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    if (vad->array_buffer_binding != 0) {
        buffer_object_t *buffer = gliGetBufferObject(vad->array_buffer_binding);
        assert(buffer != NULL);
        vad->vertex_array_buffer_binding = vad->array_buffer_binding;
    }

    vad->vertex_array_size = (GLuint)size;
    vad->vertex_array_type = type;
    vad->vertex_array_stride = (GLuint)stride;
    vad->vertex_array_ptr = ptr;

    vad->vertex_array_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glNormalPointer(GLenum type, GLsizei stride, const void *ptr)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;

    if (stride < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_BYTE && type != GL_SHORT && type != GL_FIXED && type != GL_FLOAT) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    // If a buffer is bound with glBindBuffer, ptr is treated as an offset and the bound buffer's data pointer is used
    // instead
    if (vad->array_buffer_binding != 0) {
        buffer_object_t *buffer = gliGetBufferObject(vad->array_buffer_binding);
        assert(buffer != NULL);
        vad->normal_array_buffer_binding = vad->array_buffer_binding;
    }

    vad->normal_array_ptr = ptr;
    vad->normal_array_type = type;
    vad->normal_array_stride = (GLuint)stride;
    vad->normal_array_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;

    if (stride < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_UNSIGNED_BYTE && type != GL_FIXED && type != GL_FLOAT) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (size != 4) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // If a buffer is bound with glBindBuffer, ptr is treated as an offset and the bound buffer's data pointer is used
    // instead
    if (vad->array_buffer_binding != 0) {
        buffer_object_t *buffer = gliGetBufferObject(vad->array_buffer_binding);
        assert(buffer != NULL);
        vad->color_array_buffer_binding = vad->array_buffer_binding;
    }

    vad->color_array_size = (GLuint)size;
    vad->color_array_type = type;
    vad->color_array_stride = (GLuint)stride;
    vad->color_array_ptr = ptr;

    vad->color_array_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;
    const GLenum texture = vad->client_active_texture - GL_TEXTURE0;

    if (size < 2 || size > 4) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if (type != GL_BYTE && type != GL_SHORT && type != GL_FIXED && type != GL_FLOAT) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (stride < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // If a buffer is bound with glBindBuffer, ptr is treated as an offset and the bound buffer's data pointer is used
    // instead
    if (vad->array_buffer_binding != 0) {
        buffer_object_t *buffer = gliGetBufferObject(vad->array_buffer_binding);
        if (buffer == NULL) {
            gliSetError(GL_INVALID_OPERATION);
            return;
        }
        vad->texcoord_array_buffer_binding[texture] = vad->array_buffer_binding;
    }

    vad->texcoord_array_size[texture] = (GLuint)size;
    vad->texcoord_array_type[texture] = type;
    vad->texcoord_array_stride[texture] = (GLuint)stride;
    vad->texcoord_array_ptr[texture] = ptr;

    vad->texcoord_array_dirty[texture] = GL_TRUE;
}

GL_API void GL_APIENTRY glPointSizePointerOES(GLenum type, GLsizei stride, const void *pointer)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;

    if (type != GL_FIXED && type != GL_FLOAT) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    if (stride < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // If a buffer is bound with glBindBuffer, ptr is treated as an offset and the bound buffer's data pointer is used
    // instead
    if (vad->array_buffer_binding != 0) {
        buffer_object_t *buffer = gliGetBufferObject(vad->array_buffer_binding);
        if (buffer == NULL) {
            gliSetError(GL_INVALID_OPERATION);
            return;
        }
        vad->point_size_array_buffer_binding = vad->array_buffer_binding;
    }

    vad->point_size_array_type = type;
    vad->point_size_array_stride = (GLuint)stride;
    vad->point_size_array_ptr = pointer;

    vad->point_size_array_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    gli_context_t *context = gliGetContext();

    if (count < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (mode) {
        case GL_POINTS:
        case GL_LINES:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }

    // Check if vertex array is enabled. Doesnt throw an error, just returns.
    if (context->vertex_array_data.vertex_array_enabled == GL_FALSE) {
        return;
    }

    XguPrimitiveType primitive = _gl_enum_to_xgu_primitive(mode);
    if (primitive == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    gliFlushStateChange();
    xgux_draw_arrays(primitive, first, count);
}

GL_API void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    gli_context_t *context = gliGetContext();

    if (count < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    switch (mode) {
        case GL_POINTS:
        case GL_LINES:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }

    if (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT
#ifdef GL_OES_element_index_uint
        && type != GL_UNSIGNED_INT
#endif
    ) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    XguPrimitiveType primitive = _gl_enum_to_xgu_primitive(mode);
    if (primitive == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    gliFlushStateChange();

    const void *indices_ptr;

    // If an element array buffer is bound, indices is treated as an offset into the buffer
    if (context->vertex_array_data.element_array_buffer_binding != 0) {
        buffer_object_t *buffer = gliGetBufferObject(context->vertex_array_data.element_array_buffer_binding);
        assert(buffer != NULL);
        indices_ptr = (void *)((uintptr_t)buffer->buffer_data + (uintptr_t)indices);
    } else {
        indices_ptr = indices;
    }

    if (type == GL_UNSIGNED_SHORT) {
        xgux_draw_elements16(primitive, (const uint16_t *)indices_ptr, (unsigned int)count);
    } else if (type == GL_UNSIGNED_BYTE) {
        uint16_t stack_alloc[512];
        uint16_t *element16;
        if (count <= 512) {
            element16 = stack_alloc;
        } else {
            element16 = GLI_MALLOC(sizeof(uint16_t) * count);
        }
        if (!element16) {
            gliSetError(GL_OUT_OF_MEMORY);
            return;
        }
        for (GLsizei i = 0; i < count; ++i) {
            element16[i] = ((const uint8_t *)indices_ptr)[i];
        }
        xgux_draw_elements16(primitive, element16, (unsigned int)count);
        if (element16 != stack_alloc) {
            GLI_FREE(element16);
        }
    } else {
        xgux_draw_elements32(primitive, (const uint32_t *)indices_ptr, (unsigned int)count);
    }
}

GL_API void GL_APIENTRY glMultiTexCoord4f(GLenum tex, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    gli_context_t *context = gliGetContext();
    current_values_t *cv = &context->current_values;
    vertex_array_data_t *vad = &context->vertex_array_data;

    if (tex < GL_TEXTURE0 || tex >= GL_TEXTURE0 + GLI_MAX_TEXTURE_UNITS) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    const GLenum unit = tex - GL_TEXTURE0;
    glm_vec4_copy((vec4){s, t, r, q}, context->current_values.current_texcoord[unit]);

    // This is flushed to hardware in gliTextureFlush
}

GL_API void GL_APIENTRY glMultiTexCoord4x(GLenum tex, GLfixed s, GLfixed t, GLfixed r, GLfixed q)
{
    const GLfloat fs = gliFixedtoFloat(s);
    const GLfloat ft = gliFixedtoFloat(t);
    const GLfloat fr = gliFixedtoFloat(r);
    const GLfloat fq = gliFixedtoFloat(q);
    glMultiTexCoord4f(tex, fs, ft, fr, fq);
}

GL_API void GL_APIENTRY glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    gli_context_t *context = gliGetContext();
    current_values_t *cv = &context->current_values;
    glm_vec4_copy((vec4){r, g, b, a}, cv->current_color);

    uint32_t *pb = pb_begin();
    pb = xgu_set_vertex_data4f(
        pb, XGU_COLOR_ARRAY, cv->current_color[0], cv->current_color[1], cv->current_color[2], cv->current_color[3]);
    pb_end(pb);
}

GL_API void GL_APIENTRY glColor4x(GLfixed r, GLfixed g, GLfixed b, GLfixed a)
{
    const GLfloat fr = gliFixedtoFloat(r);
    const GLfloat fg = gliFixedtoFloat(g);
    const GLfloat fb = gliFixedtoFloat(b);
    const GLfloat fa = gliFixedtoFloat(a);
    glColor4f(fr, fg, fb, fa);
}

GL_API void GL_APIENTRY glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    const GLfloat fr = (GLfloat)r / 255.0f;
    const GLfloat fg = (GLfloat)g / 255.0f;
    const GLfloat fb = (GLfloat)b / 255.0f;
    const GLfloat fa = (GLfloat)a / 255.0f;
    glColor4f(fr, fg, fb, fa);
}

GL_API void GL_APIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    gli_context_t *context = gliGetContext();
    current_values_t *cv = &context->current_values;
    glm_vec3_copy((vec3){nx, ny, nz}, cv->current_normal);

    uint32_t *pb = pb_begin();
    pb = xgu_set_vertex_data4f(
        pb, XGU_NORMAL_ARRAY, cv->current_normal[0], cv->current_normal[1], cv->current_normal[2], 0.0f);
    pb_end(pb);
}

GL_API void GL_APIENTRY glNormal3x(GLfixed nx, GLfixed ny, GLfixed nz)
{
    const GLfloat fnx = gliFixedtoFloat(nx);
    const GLfloat fny = gliFixedtoFloat(ny);
    const GLfloat fnz = gliFixedtoFloat(nz);
    glNormal3f(fnx, fny, fnz);
}

GL_API void GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum format, GLenum type, void *pixels)
{
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)format;
    (void)type;
    (void)pixels;
    // FIXME, this uses pack,unpack alignments too
    glFinish();
    // ..
}

void gliArrayFlush(void)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;
    current_values_t *cv = &context->current_values;
    const void *array_ptr = NULL;

    // Vertex
    if (vad->vertex_array_dirty) {
        XguVertexArrayType format = _gl_enum_to_xgu_type(vad->vertex_array_type);
        unsigned int stride = vad->vertex_array_stride;
        if (stride == 0) {
            stride = vad->vertex_array_size * _gl_enum_to_byte_size(vad->vertex_array_type);
        }

        // If a buffer is bound, get the actual data pointer from the buffer object
        if (vad->vertex_array_buffer_binding != 0) {
            buffer_object_t *buffer = gliGetBufferObject(vad->vertex_array_buffer_binding);
            assert(buffer != NULL);
            array_ptr = (void *)((uintptr_t)buffer->buffer_data + (uintptr_t)vad->vertex_array_ptr);
        } else {
            array_ptr = vad->vertex_array_ptr;
        }

        xgux_set_attrib_pointer(XGU_VERTEX_ARRAY, format, vad->vertex_array_size, stride, array_ptr);
        vad->vertex_array_dirty = GL_FALSE;
    }

    // Color
    if (vad->color_array_dirty) {
        if (vad->color_array_enabled) {
            XguVertexArrayType format = _gl_enum_to_xgu_type(vad->color_array_type);
            unsigned int stride = vad->color_array_stride;
            if (stride == 0) {
                stride = vad->color_array_size * _gl_enum_to_byte_size(vad->color_array_type);
            }

            // If a buffer is bound, get the actual data pointer from the buffer object then use the ptr as a offset
            if (vad->color_array_buffer_binding != 0) {
                buffer_object_t *buffer = gliGetBufferObject(vad->color_array_buffer_binding);
                assert(buffer != NULL);
                array_ptr = (void *)((uintptr_t)buffer->buffer_data + (uintptr_t)vad->color_array_ptr);
            } else {
                array_ptr = vad->color_array_ptr;
            }
            xgux_set_attrib_pointer(XGU_COLOR_ARRAY, format, vad->color_array_size, stride, array_ptr);
        } else {
            xgux_set_attrib_pointer(XGU_COLOR_ARRAY, XGU_FLOAT, 0, 0, 0);
        }
        vad->color_array_dirty = GL_FALSE;
    }

    // Normal
    if (vad->normal_array_dirty) {
        if (vad->normal_array_enabled) {
            XguVertexArrayType format = _gl_enum_to_xgu_type(vad->normal_array_type);
            unsigned int stride = vad->normal_array_stride;
            if (stride == 0) {
                stride = 3 * _gl_enum_to_byte_size(vad->normal_array_type);
            }

            // If a buffer is bound, get the actual data pointer from the buffer object then use the ptr as a offset
            if (vad->normal_array_buffer_binding != 0) {
                buffer_object_t *buffer = gliGetBufferObject(vad->normal_array_buffer_binding);
                assert(buffer != NULL);
                array_ptr = (void *)((uintptr_t)buffer->buffer_data + (uintptr_t)vad->normal_array_ptr);
            } else {
                array_ptr = vad->normal_array_ptr;
            }
            xgux_set_attrib_pointer(XGU_NORMAL_ARRAY, format, 3, stride, array_ptr);
        } else {
            xgux_set_attrib_pointer(XGU_NORMAL_ARRAY, XGU_FLOAT, 0, 0, 0);
        }
        vad->normal_array_dirty = GL_FALSE;
    }

    // Texture
    // This is flushed per-texture-unit in gliTextureFlush

    // Point Size
    if (vad->point_size_array_dirty) {
        if (vad->point_size_array_enabled) {
            XguVertexArrayType format = _gl_enum_to_xgu_type(vad->point_size_array_type);
            unsigned int stride = vad->point_size_array_stride;
            if (stride == 0) {
                stride = 1 * _gl_enum_to_byte_size(vad->point_size_array_type);
            }

            if (vad->point_size_array_buffer_binding != 0) {
                buffer_object_t *buffer = gliGetBufferObject(vad->point_size_array_buffer_binding);
                assert(buffer != NULL);
                array_ptr = (void *)((uintptr_t)buffer->buffer_data + (uintptr_t)vad->point_size_array_ptr);
            } else {
                array_ptr = vad->point_size_array_ptr;
            }
            xgux_set_attrib_pointer(XGU_POINT_SIZE_ARRAY, format, 1, stride, array_ptr);
        } else {
            xgux_set_attrib_pointer(XGU_POINT_SIZE_ARRAY, XGU_FLOAT, 0, 0, 0);
        }
        vad->point_size_array_dirty = GL_FALSE;
    }
}
