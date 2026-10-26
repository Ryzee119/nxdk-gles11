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
        case GL_FOG_COORDINATE_ARRAY_EXT:
            if (vad->fog_coord_array_enabled != enable) {
                vad->fog_coord_array_enabled = enable;
                vad->fog_coord_array_dirty = GL_TRUE;
                context->coloring_state.fog_dirty = GL_TRUE;
            }
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
        buffer_object_t *buffer = gliFindBufferObject(vad->array_buffer_binding, NULL);
        assert(buffer != NULL);
        vad->vertex_array_buffer_binding = vad->array_buffer_binding;
    } else {
        vad->vertex_array_buffer_binding = 0;
    }

    vad->vertex_array_size = size;
    vad->vertex_array_type = type;
    vad->vertex_array_stride = stride;
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
        buffer_object_t *buffer = gliFindBufferObject(vad->array_buffer_binding, NULL);
        assert(buffer != NULL);
        vad->normal_array_buffer_binding = vad->array_buffer_binding;
    } else {
        vad->normal_array_buffer_binding = 0;
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
    if (size != 4 && size != 3) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // If a buffer is bound with glBindBuffer, ptr is treated as an offset and the bound buffer's data pointer is used
    // instead
    if (vad->array_buffer_binding != 0) {
        buffer_object_t *buffer = gliFindBufferObject(vad->array_buffer_binding, NULL);
        assert(buffer != NULL);
        vad->color_array_buffer_binding = vad->array_buffer_binding;
    } else {
        vad->color_array_buffer_binding = 0;
    }

    vad->color_array_size = size;
    vad->color_array_type = type;
    vad->color_array_stride = stride;
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
        buffer_object_t *buffer = gliFindBufferObject(vad->array_buffer_binding, NULL);
        if (buffer == NULL) {
            gliSetError(GL_INVALID_OPERATION);
            return;
        }
        vad->texcoord_array_buffer_binding[texture] = vad->array_buffer_binding;
    } else {
        vad->texcoord_array_buffer_binding[texture] = 0;
    }

    vad->texcoord_array_size[texture] = size;
    vad->texcoord_array_type[texture] = type;
    vad->texcoord_array_stride[texture] = stride;
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
        buffer_object_t *buffer = gliFindBufferObject(vad->array_buffer_binding, NULL);
        if (buffer == NULL) {
            gliSetError(GL_INVALID_OPERATION);
            return;
        }
        vad->point_size_array_buffer_binding = vad->array_buffer_binding;
    } else {
        vad->point_size_array_buffer_binding = 0;
    }

    vad->point_size_array_type = type;
    vad->point_size_array_stride = stride;
    vad->point_size_array_ptr = pointer;

    vad->point_size_array_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glFogCoordPointerEXT(GLenum type, GLsizei stride, const void *pointer)
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

    if (vad->array_buffer_binding != 0) {
        buffer_object_t *buffer = gliFindBufferObject(vad->array_buffer_binding, NULL);
        if (buffer == NULL) {
            gliSetError(GL_INVALID_OPERATION);
            return;
        }
        vad->fog_coord_array_buffer_binding = vad->array_buffer_binding;
    } else {
        vad->fog_coord_array_buffer_binding = 0;
    }

    vad->fog_coord_array_type = type;
    vad->fog_coord_array_stride = stride;
    vad->fog_coord_array_ptr = pointer;

    vad->fog_coord_array_dirty = GL_TRUE;
}

GL_API void GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    gli_context_t *context = gliGetContext();

    if (count < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // Check if vertex array is enabled. Doesnt throw an error, just returns.
    if (context->vertex_array_data.vertex_array_enabled == GL_FALSE) {
        return;
    }

    DWORD primitive = gliEnumToNvPrimitive(mode);
    if (primitive == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    GLint draw_first = first;
    if (gliNeedsStaging()) {
        if (!gliHasBoundVBOArrays()) {
            if (!gliStageClientArrays(first, count)) {
                return;
            }
            draw_first = 0;
        } else {
            if (!gliStageClientArrays(0, first + count)) {
                return;
            }
        }
    }

    gliFlushStateChange();
    xgux_draw_arrays(primitive, draw_first, count);
}

GL_API void GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices)
{
    gli_context_t *context = gliGetContext();

    if (count < 0) {
        gliSetError(GL_INVALID_VALUE);
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

    DWORD primitive = gliEnumToNvPrimitive(mode);
    if (primitive == -1) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    const void *indices_ptr = gliGetBufferPointer(context->vertex_array_data.element_array_buffer_binding, indices);

    // Scan indices to find vertex range, then stage client arrays
    if (gliNeedsStaging()) {
        GLsizei max_index = gliScanMaxIndex(type, indices_ptr, count);
        if (!gliStageClientArrays(0, max_index + 1)) {
            return;
        }
    }

    gliFlushStateChange();

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

    uint32_t *pb = pb_begin();
    pb = xgu_set_vertex_data4f(pb,
                               XGU_TEXCOORD0_ARRAY + unit,
                               cv->current_texcoord[unit][0],
                               cv->current_texcoord[unit][1],
                               cv->current_texcoord[unit][2],
                               cv->current_texcoord[unit][3]);
    pb_end(pb);

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
    gli_context_t *context = gliGetContext();
    if (w < 0 || h < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if ((format != GL_ALPHA && format != GL_RGB && format != GL_RGBA) ||
        (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT_5_6_5 && type != GL_UNSIGNED_SHORT_4_4_4_4 &&
         type != GL_UNSIGNED_SHORT_5_5_5_1)) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }
    // GL_RGBA / GL_UNSIGNED_BYTE is the format reported by GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES.
    if (format != GL_RGBA || type != GL_UNSIGNED_BYTE) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }
    if (context->fbo_binding != 0) {
        // Only the default framebuffer (A8R8G8B8 back buffer) is supported.
        gliSetError(GL_INVALID_OPERATION);
        return;
    }
    if (!pixels || w == 0 || h == 0) {
        return;
    }

    glFinish();

    const GLint fbw = (GLint)pb_back_buffer_width();
    const GLint fbh = (GLint)pb_back_buffer_height();
    const size_t pitch = pb_back_buffer_pitch();
    const uint8_t *fb = (const uint8_t *)pb_back_buffer();

    const GLint bpp = 4;
    const GLint alignment = context->pixel_store.pack_alignment;
    size_t row_size = (size_t)w * bpp;
    row_size = (row_size + alignment - 1) / alignment * alignment;

    for (GLint row = 0; row < h; row++) {
        uint8_t *dst = (uint8_t *)pixels + row * row_size;
        const GLint gl_y = y + row;
        // GL's origin is bottom-left, the framebuffer's is top-left.
        const GLint fb_y = fbh - 1 - gl_y;
        for (GLint col = 0; col < w; col++, dst += bpp) {
            const GLint fb_x = x + col;
            if (fb_x < 0 || fb_x >= fbw || fb_y < 0 || fb_y >= fbh) {
                continue;
            }
            const uint8_t *src = fb + (size_t)fb_y * pitch + (size_t)fb_x * 4;
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = src[3];
        }
    }
}

static inline uint32_t *push_attrib_pointer(uint32_t *p,
                                            unsigned int index,
                                            XguVertexArrayType format,
                                            unsigned int size,
                                            unsigned int stride,
                                            const void *data)
{
    p = xgu_set_vertex_data_array_format(p, index, format, size, stride);
    p = xgu_set_vertex_data_array_offset(p, index, data ? (const void *)((uintptr_t)MmGetPhysicalAddress((PVOID)data)) : NULL);
    return p;
}

void gliArrayFlush(void)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;
    current_values_t *cv = &context->current_values;

    GLboolean any_dirty = vad->vertex_array_dirty || vad->color_array_dirty || vad->normal_array_dirty ||
                          vad->point_size_array_dirty || vad->fog_coord_array_dirty;
    if (!any_dirty) {
        for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
            if (vad->texcoord_array_dirty[i]) {
                any_dirty = GL_TRUE;
                break;
            }
        }
    }

    if (!any_dirty) {
        return;
    }

    uint32_t *pb = pb_begin();

    // Vertex
    if (vad->vertex_array_dirty) {
        XguVertexArrayType format = gliEnumToNvType(vad->vertex_array_type);
        unsigned int stride = vad->vertex_array_stride;
        if (stride == 0) {
            stride = vad->vertex_array_size * gliEnumtoByteSize(vad->vertex_array_type);
        }

        const void *array_ptr = gliGetBufferPointer(vad->vertex_array_buffer_binding, vad->vertex_array_ptr);
        pb = push_attrib_pointer(pb, XGU_VERTEX_ARRAY, format, vad->vertex_array_size, stride, array_ptr);
        vad->vertex_array_dirty = GL_FALSE;
    }

    // Color
    if (vad->color_array_dirty) {
        if (vad->color_array_enabled) {
            XguVertexArrayType format = gliEnumToNvType(vad->color_array_type);
            unsigned int stride = vad->color_array_stride;
            if (stride == 0) {
                stride = vad->color_array_size * gliEnumtoByteSize(vad->color_array_type);
            }

            const void *array_ptr = gliGetBufferPointer(vad->color_array_buffer_binding, vad->color_array_ptr);
            pb = push_attrib_pointer(pb, XGU_COLOR_ARRAY, format, vad->color_array_size, stride, array_ptr);
        } else {
            pb = push_attrib_pointer(pb, XGU_COLOR_ARRAY, XGU_FLOAT, 0, 0, NULL);
            pb = xgu_set_vertex_data4f(pb,
                                       XGU_COLOR_ARRAY,
                                       cv->current_color[0],
                                       cv->current_color[1],
                                       cv->current_color[2],
                                       cv->current_color[3]);
        }
        vad->color_array_dirty = GL_FALSE;
    }

    // Normal
    if (vad->normal_array_dirty) {
        if (vad->normal_array_enabled) {
            XguVertexArrayType format = gliEnumToNvType(vad->normal_array_type);
            unsigned int stride = vad->normal_array_stride;
            if (stride == 0) {
                stride = 3 * gliEnumtoByteSize(vad->normal_array_type);
            }

            const void *array_ptr = gliGetBufferPointer(vad->normal_array_buffer_binding, vad->normal_array_ptr);
            pb = push_attrib_pointer(pb, XGU_NORMAL_ARRAY, format, 3, stride, array_ptr);
        } else {
            pb = push_attrib_pointer(pb, XGU_NORMAL_ARRAY, XGU_FLOAT, 0, 0, NULL);
            pb = xgu_set_vertex_data4f(pb,
                                       XGU_NORMAL_ARRAY,
                                       cv->current_normal[0],
                                       cv->current_normal[1],
                                       cv->current_normal[2],
                                       0.0f);
        }
        vad->normal_array_dirty = GL_FALSE;
    }

    // Texture
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        if (vad->texcoord_array_dirty[i]) {
            const GLint xgu_slot = XGU_TEXCOORD0_ARRAY + i;
            if (vad->texcoord_array_enabled[i]) {
                XguVertexArrayType format = gliEnumToNvType(vad->texcoord_array_type[i]);
                unsigned int stride = vad->texcoord_array_stride[i];
                if (stride == 0) {
                    stride = vad->texcoord_array_size[i] * gliEnumtoByteSize(vad->texcoord_array_type[i]);
                }
                const void *array_ptr =
                    gliGetBufferPointer(vad->texcoord_array_buffer_binding[i], vad->texcoord_array_ptr[i]);
                pb = push_attrib_pointer(pb, xgu_slot, format, vad->texcoord_array_size[i], stride, array_ptr);
            } else {
                pb = push_attrib_pointer(pb, xgu_slot, XGU_FLOAT, 0, 0, NULL);
            }
            vad->texcoord_array_dirty[i] = GL_FALSE;
        }
    }

    // Point Size
    if (vad->point_size_array_dirty) {
        if (vad->point_size_array_enabled) {
            XguVertexArrayType format = gliEnumToNvType(vad->point_size_array_type);
            unsigned int stride = vad->point_size_array_stride;
            if (stride == 0) {
                stride = 1 * gliEnumtoByteSize(vad->point_size_array_type);
            }

            const void *array_ptr =
                gliGetBufferPointer(vad->point_size_array_buffer_binding, vad->point_size_array_ptr);
            pb = push_attrib_pointer(pb, XGU_POINT_SIZE_ARRAY, format, 1, stride, array_ptr);
        } else {
            pb = push_attrib_pointer(pb, XGU_POINT_SIZE_ARRAY, XGU_FLOAT, 0, 0, NULL);
        }
        vad->point_size_array_dirty = GL_FALSE;
    }

    // Fog Coordinate
    if (vad->fog_coord_array_dirty) {
        if (vad->fog_coord_array_enabled) {
            XguVertexArrayType format = gliEnumToNvType(vad->fog_coord_array_type);
            unsigned int stride = vad->fog_coord_array_stride;
            if (stride == 0) {
                stride = 1 * gliEnumtoByteSize(vad->fog_coord_array_type);
            }

            const void *array_ptr =
                gliGetBufferPointer(vad->fog_coord_array_buffer_binding, vad->fog_coord_array_ptr);
            pb = push_attrib_pointer(pb, XGU_FOG_ARRAY, format, 1, stride, array_ptr);
        } else {
            pb = push_attrib_pointer(pb, XGU_FOG_ARRAY, XGU_FLOAT, 0, 0, NULL);
        }
        vad->fog_coord_array_dirty = GL_FALSE;
    }

    pb_end(pb);
}
